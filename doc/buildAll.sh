_target=no
test "$1" && _target="$1"

#1. Build Yasm
echo "==================Build Yasm=================="
cd yasm-1.2.0
if test $_target != "clean"; then
  ./configure --prefix=/usr/local
  make -j4 && sudo make install
else
  make clean
fi

#3. Build zlib-1.2.7
cd ../zlib-1.2.7
if test $_target != "clean"; then
   CFLAGS="-O3 -fPIC" ./configure --prefix=/usr/local --static --64
  make -j4 && sudo make install
else
  make clean
fi

#4. Build libpng-1.5.18
echo "==================Build libpng-1.5.18=================="
cd ../libpng-1.5.18

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared 
  make -j4 && sudo make install
else
  make clean
fi

#5. Build freetype-2.4.10
echo "==================Build freetype-2.4.10=================="
cd ../freetype-2.4.10

if test $_target != "clean"; then
  CFLAGS="-O3 -fPIC" ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 && sudo make install
else
  make clean
fi

## Build cmake-2.8.12.2
cd ../cmake-2.8.12.2
echo "================Build cmake-2.8.12.2=================="
if test $_target != "clean"; then
  ./bootstrap
  ./configure --parallel=4
  make -j4 && sudo make install
else
  make clean
fi

#6. Build fontconfig-2.11.0
echo "==================Build fontconfig-2.11.0=================="
cd ../fontconfig-2.11.0

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared --disable-docs
  make -j4 && sudo make install
else
  make clean
fi

#7. enca
echo "==================Build enca=================="
cd ../enca

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 && sudo make install
else
  make clean
fi

#8. libexpat
echo "==================Build expat-2.1.0=================="
cd ../expat-2.1.0

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 all && sudo make install
else
  make clean
fi

#9. fribidi
echo "==================Build fribidi=================="
cd ../fribidi

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 && sudo make install
else
  make clean
fi

#10. libass
echo "==================Build libass=================="
cd ../libass-0.10.2

if test $_target != "clean"; then
  FREETYPE_CFLAGS="-I/usr/local/include/freetype2" FONTCONFIG_CFLAGS="-I/usr/local/include/fontconfig" ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 && sudo make install
else
  make clean
fi

#11. Build xvidcore
echo "==================Build xvidcore=================="
cd ../xvidcore
cd build/generic
if test $_target != "clean"; then
  CFLAGS="-O3 -fPIC" ./configure --prefix=/usr/local
  make -j4 && sudo make install
else
  make clean
fi
cd ../..

#12. Build x264
echo "==================Build x264=================="
cd ../x264

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static
  make -j4 && sudo make install
else
  make clean
fi

#12.1 Build x265
echo "==================Build x265=================="
cd ../x265


#13. Build opencore-amr-0.1.2
echo "==================Build opencore-amr-0.1.2=================="
cd ../opencore-amr-0.1.2

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --enable-static --disable-shared
  make -j4 && sudo make install
else
  make clean
fi

#14. libfdk_aac
echo "==================Build libfdk_aac=================="
#git clone --depth 1 git://github.com/mstorsjo/fdk-aac.git
cd ../fdk-aac

if test $_target != "clean"; then
  #autoreconf -fiv
  ./configure --prefix=/usr/local --disable-shared --with-pic=yes
  rm -rf ./libtool && ln -s /usr/bin/libtool ./libtool
  make -j4 && sudo make install
else
  make clean
fi

#15. Build libxml2-2.9.1
echo "==================Build libxml2-2.9.1=================="
cd ../libxml2-2.9.1

if test $_target != "clean"; then
  ./configure --prefix=/usr/local --disable-shared --enable-static --with-zlib=/usr/local
  make -j4 && sudo make install
else
  make clean
fi

#16. Build ffmpeg
echo "==================Build ffmpeg=================="
cd ../ffmpeg

if test $_target != "clean"; then
  ./autocfg-linux.sh transcli
  make -j4 && sudo make install
else
  make clean
fi
cp -f ./ffmpeg ../../transcli/bin/codecs/
cp -f ./ffprobe ../../transcli/bin/codecs/

#17. Build yamdi-1.9
echo "==================Build yamdi-1.9=================="
cd ../yamdi-1.9
if test $_target != "clean"; then
  gcc yamdi.c -o yamdi -O2 -Wall
  strip ./yamdi
  cp -f ./yamdi ../../transcli/bin/tools/
fi

#18. Build MediaInfo
cd ../MediaInfo_CLI_GNU_FromSource
./CLI_Compile.sh
strip ./MediaInfo/Project/GNU/CLI/mediainfo
cp -f ./MediaInfo/Project/GNU/CLI/mediainfo ../../transcli/bin/tools/

#19. Build MP4Box
cd ../gpac
if test $_target != "clean"; then
  make clean
  ./configure --use-js=no --use-ffmpeg=no --use-a52=no --use-openjpeg=no --use-a52=no --static-mp4box --use-zlib=local
  make lib
  cp -f ./bin/gcc/libgpac_static.a /usr/local/lib/libgpac.a
  make apps
  strip ./bin/gcc/MP4Box
  cp -f ./bin/gcc/MP4Box ../../transcli/bin/tools/
  rm -rf ./bin/gcc/libgpac.so*
else
  make clean
fi

#20 Build qtmake
# cd qt-everywhere
# ./configure
# 只需要configure就会生成qmake
# cp ./qtbase/bin/qmake ./qtbase/bin/qt.conf /usr/local/bin/
# copy mkspec文件夹 
# cp -rf ./qtbase/mkspecs /usr/local/ 
