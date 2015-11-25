#!/bin/bash

if [ ! -d "tools" ] ; then
    mkdir tools
fi

if [ ! -d "tools/raburton" ] ; then
    git clone https://github.com/raburton/esp8266.git tools/raburton
fi

if [ ! -d "tools/Sming" ] ; then
    cd tools
    wget https://github.com/SmingHub/Sming/archive/1.4.0.tar.gz
    tar xzvf 1.4.0.tar.gz Sming-1.4.0/Sming
    mv Sming-1.4.0/Sming Sming
    rm -rf Sming-1.4.0
    cd ..
fi

if [ ! -d "tools/Sming/Libraries/MySensors" ] ; then
    git clone https://github.com/alainmaes/MySensors.git tools/Sming/Libraries/MySensors
fi

cd tools/raburton/esptool2
make
cd ../../../

cd tools/Sming
sed -i 's/^#include <mem.h>/\/\/#include <mem.h>/g' rboot/appcode/rboot-api.c
export SMING_HOME=`pwd`
make rebuild
cd spiffy
make

