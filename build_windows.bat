# Build using Visual Studio 2019 on Windows
# Additional tools needed: git, cmake and ninja

# Build zlib
git clone https://github.com/madler/zlib
cd zlib
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../../

# Build libpng
git clone https://github.com/glennrp/libpng
cd libpng
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../..

# Build libsndfile - Could build Vorbis, FLAC and Opus first for extra support
git clone https://github.com/libsndfile/libsndfile.git
cd libsndfile
mkdir build
cd build
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../../winpath ..
ninja install
cd ../..

# Build aptdec
mkdir winbuild
cd winbuild
cmake -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DMSVC_TOOLSET_VERSION=190 -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath ..
ninja install
cd ../..
