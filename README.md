[![Build](https://travis-ci.org/WSNhub/MySensorsGateway.svg?branch=master)](https://travis-ci.org/WSNhub/MySensorsGateway)

# MySensorsGateway

## Install prerequisites
##### Ubuntu
```shell
sudo apt-get install make unrar autoconf automake libtool libtool-bin gcc g++ gperf flex bison texinfo gawk ncurses-dev libexpat-dev python sed python-serial srecord bc
```

## Build esp-open-sdk
(Or get [precompiled esp-open-sdk](https://bintray.com/artifact/download/kireevco/generic/esp-open-sdk-linux-1.0.1.tar.gz))
```
git clone https://github.com/pfalcon/esp-open-sdk.git
cd esp-open-sdk
make STANDALONE=y # It will take a while...
```

## Set ENV Variables
```shell
export ESP_HOME="/opt/esp-open-sdk";
```

## Project Setup
```shell
./setup.sh
```

## Configuration
You might want to configure your project before building.
Edit **Makefile-user.mk**

## Building
```shell
make
```

## Flashing your ESP
```shell
make flash
```

## ToDo
Add OTA (from rboot sample)

Add MQTT (from MQTT sample)
