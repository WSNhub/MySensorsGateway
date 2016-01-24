#!/bin/bash

if [ ! -f "app/globals.c" ] ; then
    touch "app/globals.c"
fi

if [ ! -d "tools" ] ; then
    mkdir tools
fi

if [ ! -d "tools/raburton" ] ; then
    git clone https://github.com/raburton/esp8266.git tools/raburton
fi

if [ ! -d "tools/esptool2" ] ; then
    git clone https://github.com/raburton/esptool2.git tools/esptool2
fi

if [ ! -d "tools/Sming" ] ; then
    cd tools
    wget https://github.com/SmingHub/Sming/archive/2.1.1.tar.gz
    tar xzvf 2.1.1.tar.gz Sming-2.1.1/Sming
    mv Sming-2.1.1/Sming Sming
    rm -rf Sming-2.1.1
    rm -rf *.tar.gz
    cd ..
fi

if [ ! -d "tools/Sming/Libraries/MySensors" ] ; then
    git clone https://github.com/WSNhub/MySensors.git tools/Sming/Libraries/MySensors
fi

if [ ! -d "tools/Sming/Libraries/MyInterpreter" ] ; then
    git clone https://github.com/WSNhub/MyInterpreter.git tools/Sming/Libraries/MyInterpreter
fi

if [ ! -e "app/mutex.c" ]; then
    ln -s ../tools/raburton/mutex/mutex.c app/mutex.c
fi

if [ ! -e "app/mutex.h" ]; then
    ln -s ../tools/raburton/mutex/mutex.h app/mutex.h
fi

# Now apply patches if any is present
patch -p0 < patches/CommandHandler.patch

cd tools/esptool2
make
cd ../../

cd tools/Sming
export SMING_HOME=`pwd`
make rebuild
cd spiffy
make

