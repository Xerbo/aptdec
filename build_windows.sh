WINE_LIBSNDFILE_PATH=~/.wine/drive_c/Program\ Files/Mega-Nerd/libsndfile
TEMP_PATH=/tmp/windows_build

# Compile zlib
git clone https://github.com/madler/zlib && cd zlib
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
make -j4
make install
cd ../../

# Compile libpng
git clone https://github.com/glennrp/libpng && cd libpng
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
make -j4
make install
cd ../../

# Download libsndfile (compiling from source is an absolute bitch)
if [[ ! -e $WINE_LIBSNDFILE_PATH ]]; then
	wget http://www.mega-nerd.com/libsndfile/files/libsndfile-1.0.28-w64-setup.exe
	echo "This build script has guessed that you don't have libsndfile installed under wine, the libsndfile installer will be launched"
	read
	wine libsndfile-1.0.28-w64-setup.exe
fi
if [[ ! -e $WINE_LIBSNDFILE_PATH ]]; then
	echo "Something went wrong installing libsndfile"
	exit
fi

cp "$WINE_LIBSNDFILE_PATH/lib/libsndfile-1.def"     $TEMP_PATH/lib/libsndfile-1.def
cp "$WINE_LIBSNDFILE_PATH/lib/libsndfile-1.lib"     $TEMP_PATH/lib/libsndfile-1.lib
cp "$WINE_LIBSNDFILE_PATH/lib/pkgconfig/sndfile.pc" $TEMP_PATH/lib/pkgconfig/sndfile.pc
cp "$WINE_LIBSNDFILE_PATH/bin/libsndfile-1.dll"     $TEMP_PATH/bin/libsndfile-1.dll
cp "$WINE_LIBSNDFILE_PATH/include/sndfile.h"        $TEMP_PATH/include/sndfile.h

sed -i "s/c:\/devel\/target\/libsndfile/$(echo $TEMP_PATH | sed 's/\//\\\//g')/g" $TEMP_PATH/lib/pkgconfig/sndfile.pc

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain-mingw32.cmake -DCMAKE_INSTALL_PREFIX=$TEMP_PATH ..
cp $TEMP_PATH/bin/*.dll ./

echo "Done, you should have a executable called aptdec.exe"
