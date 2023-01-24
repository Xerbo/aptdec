#!/usr/bin/env bash
# This script only works on Debian Bullseye
# If you have docker this is trivially easy:
#   docker run -v $(pwd):/aptdec:z -w /aptdec debian:11 ./build_arm.sh

apt-get update
apt-get install -y debootstrap cmake gcc-arm-linux-gnueabihf

# Prepare armhf root environment
if [ ! -d root ]; then
    debootstrap --keyring=/usr/share/keyrings/debian-archive-keyring.gpg --arch=armhf --include=libsndfile1-dev,libpng-dev --download-only bullseye root http://deb.debian.org/debian/
    for i in root/var/cache/apt/archives/*.deb; do dpkg -x "$i" root; done
fi

# Build aptdec
cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-armhf.cmake -DCMAKE_INSTALL_PREFIX=/aptdec/root
cmake --build build -j$(nproc)
cmake --build build --target package
