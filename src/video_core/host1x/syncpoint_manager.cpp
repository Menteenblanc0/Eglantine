// SPDX-FileCopyrightText: Copyright 2026 Eglantine Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: Copyright 2025 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

// SPDX-FileCopyrightText: 2021 yuzu Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#include "common/logging.h"
#include "common/scope_exit.h"
#include "video_core/host1x/syncpoint_manager.h"

namespace Tegra {

namespace Host1x {

// Set while this thread runs due actions; a cancel from inside one must not wait for itself.
struct RunningActions {
    const SyncpointManager* manager;
    const RunningActions* previous;
};
static thread_local const RunningActions* running_actions_on_thread{};

// The puller's fence action and the 3D sync_info register take the syncpoint id from the guest's
// command stream (24 and 16 bits): an id past the table indexed unrelated memory as an action list
// and crashed in Increment.
static bool IsValidSyncpoint(u32 syncpoint_id) {
    if (syncpoint_id < SyncpointManager::NUM_MAX_SYNCPOINTS) {
        return true;
    }
    static std::atomic_flag logged;
    if (!logged.test_and_set()) {
        LOG_ERROR(HW_GPU, "Syncpoint {} is out of range, increment dropped (logged once)",
                  syncpoint_id);
    }
    return false;
}

SyncpointManager::ActionHandle SyncpointManager::RegisterAction(
    std::atomic<u32>& syncpoint, std::list<RegisteredAction>& action_storage, u32 expected_value,
    std::function<void()>&& action) {
    if (syncpoint.load(std::memory_order_acquire) >= expected_value) {
        action();
        return {};
    }

    std::unique_lock lk(guard);
    if (syncpoint.load(std::memory_order_relaxed) >= expected_value) {
        lk.unlock();
        action();
        return {};
    }
    auto it = action_storage.begin();
    while (it != action_storage.end()) {
        if (it->expected_value > expected_value) {
            break;
        }
        ++it;
    }
    return action_storage.emplace(it, expected_value, ++last_action_id, std::move(action))->id;
}

void SyncpointManager::DeregisterAction(std::list<RegisteredAction>& action_storage,
                                        const ActionHandle& handle) {
    if (handle == 0) {
        return; // It ran inside RegisterAction
    }
    std::unique_lock lk(guard);

    for (auto it = action_storage.begin(); it != action_storage.end(); it++) {
        if (it->id == handle) {
            action_storage.erase(it);
            return;
        }
    }
    // Not registered any more: it may be due and about to run outside the guard. Return only once
    // it ran, so a cancelled action never fires after its cancel, as when actions ran under it.
    for (auto* active = running_actions_on_thread; active; active = active->previous) {
        if (active->manager == this) {
            return;
        }
    }
    actions_done_cv.wait(lk, [this] { return running_actions == 0; });
}

void SyncpointManager::DeregisterGuestAction(u32 syncpoint_id, const ActionHandle& handle) {
    DeregisterAction(guest_action_storage[syncpoint_id], handle);
}

void SyncpointManager::DeregisterHostAction(u32 syncpoint_id, const ActionHandle& handle) {
    DeregisterAction(host_action_storage[syncpoint_id], handle);
}

void SyncpointManager::IncrementGuest(u32 syncpoint_id) {
    if (!IsValidSyncpoint(syncpoint_id)) {
        return;
    }
    Increment(syncpoints_guest[syncpoint_id], wait_guest_cv, guest_action_storage[syncpoint_id],
              syncpoint_id);
}

void SyncpointManager::SetGuestMirror(GuestMirror mirror) {
    std::scoped_lock lk(guard);
    guest_mirror = std::move(mirror);
    if (guest_mirror) {
        for (u32 id = 0; id < NUM_MAX_SYNCPOINTS; ++id) {
            guest_mirror(id, syncpoints_guest[id].load(std::memory_order_acquire));
        }
    }
}

void SyncpointManager::IncrementHost(u32 syncpoint_id) {
    if (!IsValidSyncpoint(syncpoint_id)) {
        return;
    }
    Increment(syncpoints_host[syncpoint_id], wait_host_cv, host_action_storage[syncpoint_id]);
}

void SyncpointManager::WaitGuest(u32 syncpoint_id, u32 expected_value) {
    Wait(syncpoints_guest[syncpoint_id], wait_guest_cv, expected_value);
}

void SyncpointManager::WaitHost(u32 syncpoint_id, u32 expected_value) {
    Wait(syncpoints_host[syncpoint_id], wait_host_cv, expected_value);
}

void SyncpointManager::Increment(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv,
                                 std::list<RegisteredAction>& action_storage,
                                 std::optional<u32> mirrored_id) {
    // The due actions run after the guard is released: the composite action can block on the
    // display (AcquireNextImage), and every other syncpoint user (the fencing thread's Increment,
    // the guest's RegisterAction) then waited on the guard behind it, freezing Donkey Kong
    // Bananza. Splicing keeps their value order.
    std::list<RegisteredAction> due;
    {
        std::scoped_lock lk(guard);
        const u32 new_value = syncpoint.load(std::memory_order_relaxed) + 1;
        if (mirrored_id && guest_mirror) {
            guest_mirror(*mirrored_id, new_value);
        }
        // Publish only after the mirror: ready checks may run without the guard.
        syncpoint.store(new_value, std::memory_order_release);
        if (mirrored_id) {
            // After the mirror: whoever takes the bit finds the new value in the page.
            guest_moved[*mirrored_id / 64].fetch_or(u64{1} << (*mirrored_id % 64),
                                                    std::memory_order_acq_rel);
        }
        auto it = action_storage.begin();
        while (it != action_storage.end() && it->expected_value <= new_value) {
            due.splice(due.end(), action_storage, it++);
        }
        running_actions += due.empty() ? 0 : 1;
        wait_cv.notify_all();
    }
    if (due.empty()) {
        return;
    }
    const RunningActions active{this, running_actions_on_thread};
    running_actions_on_thread = &active;
    SCOPE_EXIT {
        running_actions_on_thread = active.previous;
        std::scoped_lock lk(guard);
        --running_actions;
        actions_done_cv.notify_all();
    };
    for (auto& registered : due) {
        registered.action();
    }
}

void SyncpointManager::Wait(std::atomic<u32>& syncpoint, std::condition_variable& wait_cv,
                            u32 expected_value) {
    const auto pred = [&]() { return syncpoint.load(std::memory_order_acquire) >= expected_value; };
    if (pred()) {
        return;
    }

    std::unique_lock lk(guard);
    wait_cv.wait(lk, pred);
}

} // namespace Host1x

} // namespace Tegra
