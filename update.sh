#!/bin/bash

cd tools/Sming
git pull
make rebuild
cd spiffy
make
cd ../../../
git pull
make clean
make
