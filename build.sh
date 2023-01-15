#!/bin/bash
set -e
set -x

USE_DLOPEN="yes"

if [ ! -f libsteam_api.so ]; then
  wget -O libsteam_api.so https://github.com/darealshinji/Steam-files-and-notes/raw/master/libsteam_api64.so
fi

if [ ! -d fltk ]; then
  git clone https://github.com/fltk/fltk
fi

if [ ! -d fltk/build/usr ]; then
  mkdir fltk/build
  pushd fltk/build
  cmake .. -DCMAKE_BUILD_TYPE=Release \
    -DFLTK_BUILD_TEST=OFF \
    -DCMAKE_INSTALL_PREFIX="$PWD/usr" \
    -DOPTION_OPTIM="-O3 -ffunction-sections -fdata-sections"
  make -j4
  make install
  popd
fi

CONFIG="fltk/build/usr/bin/fltk-config"
CFLAGS="-Wall -O3 $($CONFIG --cxxflags)"
LDFLAGS="-Wl,--gc-sections -Wl,--as-needed $($CONFIG --use-images --ldflags)"

if [ "x$USE_DLOPEN" != "x" ]; then
  CFLAGS="$CFLAGS -DUSE_DLOPEN=1"
else
  LDFLAGS="$LDFLAGS -L. -lsteam_api"
fi

g++ $CFLAGS -o steam_taskbar steam_taskbar.cpp $LDFLAGS -s -Wl,-rpath,'$ORIGIN'
ln -fs steam_taskbar steam_taskbar_idle
