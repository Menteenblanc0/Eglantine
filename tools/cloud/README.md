# Linux cloud validation

This image supplies build dependencies without installing packages on the host. Build it from
the existing isolated checkout; no extra Git worktree is needed:

```sh
docker build -t eglantine-validation tools/cloud
```

Run commands with the checkout mounted at `/workspace/Eglantine`. On the managed cloud machine,
use the local Docker socket explicitly and preserve its proxy and certificate configuration:

```sh
env -u DOCKER_HOST -u DOCKER_CONTEXT -u DOCKER_TLS -u DOCKER_TLS_VERIFY -u DOCKER_CERT_PATH \
docker --host=unix:///var/run/docker.sock run --rm \
  --mount "type=bind,src=$PWD,dst=/workspace/Eglantine" \
  --mount type=bind,src=/etc/ssl/certs/ca-certificates.crt,dst=/run/session-ca.crt,readonly \
  -e SSL_CERT_FILE=/run/session-ca.crt -e CURL_CA_BUNDLE=/run/session-ca.crt \
  -e GIT_SSL_CAINFO=/run/session-ca.crt \
  -e GIT_CONFIG_COUNT=1 -e GIT_CONFIG_KEY_0=safe.directory \
  -e GIT_CONFIG_VALUE_0=/workspace/Eglantine \
  eglantine-validation cmake -S . -B build-orchid-validation -G Ninja \
    -DCMAKE_BUILD_TYPE=Release '-DCMAKE_CXX_FLAGS_RELEASE=-O2 -DNDEBUG' \
    -DENABLE_LTO=OFF -DBUILD_TESTING=ON -DYUZU_TESTS=ON -DYUZU_CMD=ON \
    -DYUZU_ROOM=OFF -DYUZU_ROOM_STANDALONE=OFF \
    -DYUZU_USE_BUNDLED_FFMPEG=ON -DYUZU_USE_BUNDLED_SDL3=ON
```

With the same container options, build with
`cmake --build build-orchid-validation --parallel 4 --target tests yuzu yuzu-cmd`.
Run `build-orchid-validation/bin/tests '[ampere]'` and the NUP, loose-directory, syncpoint,
GPU-dirty-memory, buffer-cache and texture regressions listed in the implementation report.
Live processes are not part of the prepared filesystem: start the executable again when needed.

## Timezone data for validation

The default pinned download from `git.eden-emu.dev` remains unchanged. If this destination is
blocked, request access to it for a normal build. The supported `YUZU_TZDB_PATH` option can instead
point at an existing directory of **Nintendo-format** timezone data; host `/usr/share/zoneinfo`
does not substitute for that format.

The initial cloud validation used the historical official `221202` package (2022g), also referenced
by repository commit `25708d51c9`, through this override. It was downloaded with TLS verification
from <https://github.com/lat9nq/tzdb_to_nx/releases/download/221202/221202.zip>. Its SHA-256 is
`991cd6fa2230ad4d73b311e67fed1102b53c052840a0432f0b3f8c7171452b77`.
An extracted copy can live under `.cache/cloud-validation/tzdb-221202`; pass its corresponding
container path with `-DYUZU_TZDB_PATH=...`. This is a validation fallback with older timezone
rules, not a replacement of the locked `230326` dependency or proof of release parity.

No console keys, saves, game files or user home directory need to be mounted for these tests.
