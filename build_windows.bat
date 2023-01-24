REM Build using MSVC on Windows
REM Requires: git, cmake and ninja
REM You need to run vcvars before running this

REM Build zlib
IF NOT EXIST zlib (
    git clone --depth 1 -b v1.2.13 https://github.com/madler/zlib
    cd zlib
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath
    cmake --build build -j%NUMBER_OF_PROCESSORS%
    cmake --build build --target install
    cd ..
)

REM Build libpng
IF NOT EXIST libpng (
    git clone --depth 1 -b v1.6.39 https://github.com/glennrp/libpng
    cd libpng
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath -DPNG_STATIC=OFF -DPNG_EXECUTABLES=OFF -DPNG_TESTS=OFF
    cmake --build build -j%NUMBER_OF_PROCESSORS%
    cmake --build build --target install
    cd ..
)

REM Build libsndfile, only with WAV support
IF NOT EXIST libsndfile (
    git clone --depth 1 -b 1.2.0 https://github.com/libsndfile/libsndfile
    cd libsndfile
    cmake -B build -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF -DBUILD_PROGRAMS=OFF
    cmake --build build -j%NUMBER_OF_PROCESSORS%
    cmake --build build --target install
    cd ..
)

REM Build aptdec
cmake -B winbuild -G Ninja -DCMAKE_C_COMPILER="cl.exe" -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=../winpath
cmake --build winbuild -j%NUMBER_OF_PROCESSORS%
cmake --build winbuild --target package
