REM Build using MSVC on Windows
REM Requires: git, cmake and ninja

REM Build zlib
IF NOT EXIST zlib (
    git clone -b v1.2.13 https://github.com/madler/zlib
    cd zlib
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath
    cmake --build build -j4
    cmake --build build --target install
    cd ..
)

REM Build libpng
IF NOT EXIST libpng (
    git clone -b v1.6.39 https://github.com/glennrp/libpng
    cd libpng
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath -DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF
    cmake --build build -j4
    cmake --build build --target install
    cd ..
)

REM Build libsndfile, only with WAV support
IF NOT EXIST libsndfile (
    git clone -b 1.2.0 https://github.com/libsndfile/libsndfile
    cd libsndfile
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath -BUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF -DBUILD_PROGRAMS=OFF
    cmake --build build -j4
    cmake --build build --target install
    cd ..
)

REM Build aptdec
cmake -B winbuild -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath
cmake --build winbuil -j4
