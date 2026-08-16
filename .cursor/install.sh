#!/usr/bin/env bash
#
# Cloud Agent install step for the All-Projects repository.
#
# Prepares the DINO_GAME LVGL + SDL2 desktop simulator so it can be built and
# run headlessly. GUI_GSP is an embedded GUI library that is ported into
# firmware and has no standalone desktop build target, so nothing is built for
# it here.
#
# This script is idempotent: apt installs are no-ops when packages are already
# present, and the CMake configure/build steps reuse existing build state.
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# --- System packages -------------------------------------------------------
# build-essential is required because LVGL's CMake project enables C++ and the
# default image's `c++`/`cc` alias (clang) cannot locate libstdc++ on its own.
# libsdl2-dev provides the SDL2 backend used by the LVGL simulator. The Xvfb /
# xdotool / ImageMagick tools let agents run and screenshot the GUI simulator
# on a headless VM.
sudo apt-get update -qq
sudo DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    pkg-config \
    libsdl2-dev \
    xvfb \
    x11-utils \
    xdotool \
    imagemagick

# --- Build the DINO_GAME simulator ----------------------------------------
# CMake FetchContent clones LVGL v9.2.2. gcc/g++ are pinned explicitly so the
# C++-enabled LVGL project links against libstdc++ (the image aliases c++ to
# clang, which fails to find libstdc++).
cd "${REPO_ROOT}/DINO_GAME"
cmake -S . -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_COMPILER=gcc \
    -DCMAKE_CXX_COMPILER=g++
cmake --build build -j"$(nproc)"

echo "install.sh: DINO_GAME built at ${REPO_ROOT}/DINO_GAME/build/dino_game"
