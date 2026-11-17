FROM debian:trixie-slim
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    autoconf cmake g++ gcc git glslang-tools libglu1-mesa-dev libpulse-dev libtool \
    libudev-dev libxkbcommon-x11-0 mesa-common-dev nasm ninja-build qt6-base-private-dev \
    qt6-charts-dev qt6-tools-dev libdrm-dev libx11-dev pkg-config zlib1g-dev libva-dev \
    libvdpau-dev spirv-tools spirv-headers libusb-1.0-0-dev libboost-dev \
    libboost-fiber-dev libboost-context-dev libopus-dev libasound2-dev ccache \
    libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1 libxcb-randr0 \
    libxcb-render-util0 libxcb-shape0 libxcb-xinerama0 libxcb-xkb1 libssl-dev \
    libzstd-dev ca-certificates curl liblz4-dev nlohmann-json3-dev xvfb xauth \
    libvulkan-dev && rm -rf /var/lib/apt/lists/*
WORKDIR /workspace/Eglantine

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends patch python3 && rm -rf /var/lib/apt/lists/*
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends x11-utils && rm -rf /var/lib/apt/lists/*
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    mesa-vulkan-drivers libgl1-mesa-dri libegl-mesa0 && rm -rf /var/lib/apt/lists/*
ENV LANG=C.UTF-8
