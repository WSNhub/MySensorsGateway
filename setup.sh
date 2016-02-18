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
    git checkout master
    git remote add upstream https://github.com/SmingHub/Sming
    cd ..
    #wget https://github.com/SmingHub/Sming/archive/2.1.1.tar.gz
    #tar xzvf 2.1.1.tar.gz Sming-2.1.1/Sming
    #mv Sming-2.1.1/Sming Sming
    #rm -rf Sming-2.1.1
    #rm -rf *.tar.gz
    cd ..
fi

if [ ! -d "tools/Sming/Sming/Libraries/MySensors" ] ; then
    git clone https://github.com/WSNhub/MySensors.git tools/Sming/Sming/Libraries/MySensors
fi

if [ ! -d "tools/Sming/Sming/Libraries/MyInterpreter" ] ; then
    git clone https://github.com/WSNhub/MyInterpreter.git tools/Sming/Sming/Libraries/MyInterpreter
fi

cd tools/esptool2
make
cd ../../

cd tools/Sming/Sming
export SMING_HOME=`pwd`
make rebuild
cd spiffy
make

