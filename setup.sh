#!/bin/bash

if [ ! -f "app/globals.c" ] ; then
    touch "app/globals.c"
fi

if [ ! -d "tools" ] ; then
    mkdir tools
fi

if [ ! -d "tools/esptool2" ] ; then
    git clone https://github.com/raburton/esptool2.git tools/esptool2
fi

if [ ! -d "tools/Sming" ] ; then
    cd tools
    git clone https://github.com/WSNhub/Sming
    cd Sming
    git checkout develop
    git remote add upstream https://github.com/SmingHub/Sming
    cd ..
    #wget https://github.com/SmingHub/Sming/archive/2.1.1.tar.gz
    #tar xzvf 2.1.1.tar.gz Sming-2.1.1/Sming
    #mv Sming-2.1.1/Sming Sming
    #rm -rf Sming-2.1.1
    #rm -rf *.tar.gz
    cd ..
fi

cd tools/esptool2
make
cd ../../

cd tools/Sming/Sming
export SMING_HOME=`pwd`
make rebuild
cd spiffy
make

