#!/bin/bash

cd tools/Sming/Libraries/MySensors
git pull

cd ../MyInterpreter
git pull

cd ../../
make rebuild

cd spiffy
make

cd ../../../

git pull
make clean
make
