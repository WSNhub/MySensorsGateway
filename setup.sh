#!/bin/bash

if [ ! -f "app/globals.c" ] ; then
    touch "app/globals.c"
fi

cd tools/esptool2
make
cd ../../

cd tools/Sming/Sming
export SMING_HOME=`pwd`
make rebuild
cd spiffy
make
