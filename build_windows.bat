REM Build using Visual Studio 2019 on Windows
REM Additional tools needed: git, cmake and ninja

REM Build zlib
git clone -b v1.2.13 https://github.com/madler/zlib
cd zlib
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../../

REM Build libpng
git clone -b v1.6.39 https://github.com/glennrp/libpng
cd libpng
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../..

REM Build libsndfile - Could build Vorbis, FLAC and Opus first for extra support
git clone -b 1.1.0 https://github.com/libsndfile/libsndfile
cd libsndfile
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../..

REM Build aptdec
mkdir winbuild
cd winbuild
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath ..
ninja install
cd ..
