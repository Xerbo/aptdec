#!/usr/bin/env bash
# Cross compile for Windows from Linux
set -e

TEMP_PATH="$(pwd)/winpath"
BUILD_DIR="winbuild"

# Build zlib from source
if [ ! -d "zlib" ]; then
    git clone --depth 1 -b v1.2.13 https://github.com/madler/zlib && cd zlib
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH
    cmake --build build -j$(nproc)
    cmake --build build --target install
    cd ..
fi

# Build libpng from source
if [ ! -d "libpng" ]; then
    git clone --depth 1 -b v1.6.39 https://github.com/glennrp/libpng && cd libpng
    cmake -B build -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH -DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF
    cmake --build build -j$(nproc)
    cmake --build build --target install
    cd ..
fi

# Download libsndfile
if [ ! -d libsndfile-1.2.0-win64 ]; then
    wget https://github.com/libsndfile/libsndfile/releases/download/1.2.0/libsndfile-1.2.0-win64.zip
    unzip libsndfile-1.2.0-win64.zip
    cp "libsndfile-1.2.0-win64/bin/sndfile.dll"   $TEMP_PATH/bin
    cp "libsndfile-1.2.0-win64/include/sndfile.h" $TEMP_PATH/include
    cp "libsndfile-1.2.0-win64/lib/sndfile.lib"   $TEMP_PATH/lib
fi

find_dll() {
    filename=$(x86_64-w64-mingw32-gcc -print-file-name=$1)
    if [ -f $filename ]; then
        echo $filename
    else
        filename=$(x86_64-w64-mingw32-gcc -print-sysroot)/mingw/bin/$1
        if [ -f $filename ]; then
            echo $filename
        else
            echo "Could not find $1" >&2
            return 1
        fi
    fi
}

# Copy required GCC libs
cp $(find_dll libgcc_s_seh-1.dll)  $TEMP_PATH/bin
cp $(find_dll libwinpthread-1.dll) $TEMP_PATH/bin

# Build aptdec
cmake -B $BUILD_DIR -DCMAKE_BUILD_TYPE=$1 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH
cmake --build $BUILD_DIR -j$(nproc)
cmake --build $BUILD_DIR --target package
