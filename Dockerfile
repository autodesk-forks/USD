FROM ubuntu:23.04
SHELL ["/bin/bash", "-c"]
ARG BUILD_TARGET="--build-target wasm"
ARG EMSCRIPTEN_VERSION=3.1.60
RUN apt-get -y update && apt-get install -y\
        software-properties-common \
        g++ \
        lbzip2 \
        git \
        npm=9.2.*

# Install required tools for building CMake
RUN apt-get install -y wget libssl-dev

# Remove existing cmake
RUN apt-get purge -y cmake

# Download and install CMake 3.27.9 precompiled binary
RUN wget https://github.com/Kitware/CMake/releases/download/v3.27.9/cmake-3.27.9-Linux-x86_64.sh && \
    chmod +x cmake-3.27.9-Linux-x86_64.sh && \
    ./cmake-3.27.9-Linux-x86_64.sh --skip-license --prefix=/usr/local

RUN alias python='python3'
RUN mkdir -p tmp && cd tmp && git clone --recursive https://github.com/emscripten-core/emsdk
RUN cd /tmp/emsdk && ./emsdk install ${EMSCRIPTEN_VERSION}
RUN cd /tmp/emsdk && ./emsdk activate ${EMSCRIPTEN_VERSION} --permanent

COPY . /usd/
RUN source /tmp/emsdk/emsdk_env.sh && cd /usd && python3 ./build_scripts/build_usd.py -v ${BUILD_TARGET} --js-bindings USD_emscripten

