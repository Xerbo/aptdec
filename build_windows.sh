TEMP_PATH="$(pwd)/winpath"
set -e

# Compile and build zlib
if [ -d "zlib" ]; then
    cd zlib && git pull
else
    git clone https://github.com/madler/zlib && cd zlib
fi

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
make -j4
make install
cd ../..

# Clone and build ligpng
if [ -d "libpng" ]; then
    cd libpng && git pull
else
    git clone https://github.com/glennrp/libpng && cd libpng
fi

mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
make -j4
make install
cd ../..

# Download libsndfile
if [ ! -d "libsndfile-1.0.29-win64" ]; then
    wget https://github.com/erikd/libsndfile/releases/download/v1.0.29/libsndfile-1.0.29-win64.zip
    unzip libsndfile-1.0.29-win64.zip
fi
cp "libsndfile-1.0.29-win64/bin/sndfile.dll"   $TEMP_PATH/bin/libsndfile-1.dll
cp "libsndfile-1.0.29-win64/include/sndfile.h" $TEMP_PATH/include/sndfile.h

# Copy DLL's into root for CPack
cp $TEMP_PATH/bin/*.dll ../
mv ../libsndfile-1.dll ../sndfile.dll

# Build aptdec
mkdir -p winbuild && cd winbuild
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
make -j 4
make package
