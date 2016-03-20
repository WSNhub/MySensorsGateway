###################################
#### MySensorsGateway Makefile ####
###################################

# Including user Makefile.
# Should be used to set project-specific parameters
include ./Makefile-user.mk

#################################################
## APPLICATION defaults                        ##
## These can be overridden in Makefile-user.mk ##
#################################################

# PLATFORM_TYPE
# The platform type defines the accepted hardware and defines the pins
# used to attach the hardware to. See below for possibilities.
PLATFORM_TYPE ?= GENERIC

# CONTROLLER_TYPE
# For now the only sane option is OPENHAB
# Alternative is CLOUD but not generally available
CONTROLLER_TYPE ?= OPENHAB

# MYSENSORS_SIGNING
# For now better leave this disabled
MYSENSORS_SIGNING ?= 0

# MYSENSORS_SIGNING_HMAC
# If signing is enabled and no ATSHA204 IC is detected signing has to
# be performed in software. For that an HMAC key is needed. It can be
# defined here. This key is shared with your nodes and MUST match.
MYSENSORS_SIGNING_HMAC ?= { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08, \
                            0x09,0x00,0x00,0x00,0x00,0x00,0x00,0x00, \
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00, \
                            0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 }

# WIRED_ETHERNET
# Instead of the wifi station to connect to your network, it is possible
# to connect to the network over wired ethernet.
# Possibilities are:
#   WIRED_ETHERNET_NONE
#   WIRED_ETHERNET_W5100
#   WIRED_ETHERNET_W5500
WIRED_ETHERNET_MODE ?= WIRED_ETHERNET_NONE

# SMING_AUTO_UPGRADE
# If enabled, each time "make" is done, the system will check whether
# upgrading Sming is necessary. A reason to disable this would be if
# your computer does not have an internet connection all the time.
# Does that still exist?
SMING_AUTO_UPGRADE ?= 1

# GPIO16_MEASURE_ENABLE
# For debugging purpose it might be more useful to toggle a pin and
# monitor that with a logic analyzer rather than adding prints.
GPIO16_MEASURE_ENABLE ?= 0


#########################
## Platform definition ##
#########################

## Generic platform
ifeq ('${PLATFORM_TYPE}', 'GENERIC')
  USER_CFLAGS += "-DRADIO_CE_PIN=2"
  USER_CFLAGS += "-DRADIO_SPI_SS_PIN=15"
  USER_CFLAGS += "-DI2C_SDA_PIN=4"
  USER_CFLAGS += "-DI2C_SCL_PIN=5"
  USER_CFLAGS += "-DRTC_TYPE=RTC_TYPE_3213"
  USER_CFLAGS += "-DETHERNET_SPI_SS_PIN=16"
endif

## SD shield platform
## Will be removed soon
ifeq ('${PLATFORM_TYPE}', 'SDSHIELD')
  USER_CFLAGS += "-DRADIO_CE_PIN=2"
  USER_CFLAGS += "-DRADIO_SPI_SS_PIN=15"
  USER_CFLAGS += "-DSD_SPI_SS_PIN=0"
  USER_CFLAGS += "-DI2C_SDA_PIN=4"
  USER_CFLAGS += "-DI2C_SCL_PIN=5"
  USER_CFLAGS += "-DRTC_TYPE=RTC_TYPE_1307"
  USER_CFLAGS += "-DETHERNET_SPI_SS_PIN=16"
endif

## WeMos D1 board wit W5100 ethernet shield and data logger shield
## The W5100 ethernet shield has a micro SD slot BUT it trashed the SPI
## bus so it can not be used. By adding the data logger shield an SD slot
## is available and it provides an RTC also. Not the best RTC ever but it
## will do for prototyping.
ifeq ('${PLATFORM_TYPE}', 'WEMOS_WITH_SHIELDS')
  USER_CFLAGS += "-DRADIO_CE_PIN=2"
  USER_CFLAGS += "-DRADIO_SPI_SS_PIN=16"
  USER_CFLAGS += "-DSD_SPI_SS_PIN=0"
  USER_CFLAGS += "-DI2C_SDA_PIN=4"
  USER_CFLAGS += "-DI2C_SCL_PIN=5"
  USER_CFLAGS += "-DRTC_TYPE=RTC_TYPE_1307"
  USER_CFLAGS += "-DETHERNET_SPI_SS_PIN=15"
endif


#############################################
#### Please don't change anything below. ####
#############################################

all: SMING GLOBALS spiff_clean

GLOBALS:
	@echo "Generating globals"
	git describe --abbrev=7 --dirty --always --tags | awk ' BEGIN {print "#include \"globals.h\""} {print "const char * build_git_sha = \"" $$0"\";"} END {}' > app/globals.c
	date | awk 'BEGIN {} {print "const char * build_time = \""$$0"\";"} END {} ' >> app/globals.c


SMING:
ifeq ($(SMING_AUTO_UPGRADE), 1)
	@echo "Updating Sming..."
	@cd tools/Sming; git checkout develop; git pull || (echo "Sming needs rebuild"; cd Sming; make clean)
endif

	@echo "Building Sming..."
	@make rebuild -C tools/Sming/Sming
	@make -C tools/Sming/Sming/spiffy

#Add your source directories here separated by space
MODULES         = app $(filter %/, $(wildcard libraries/*/))
EXTRA_INCDIR    = include libraries

## ESP_HOME sets the path where ESP tools and SDK are located.
ESP_HOME ?= /opt/esp-open-sdk

## SMING_HOME sets the path where Sming framework is located.
SMING_HOME = ${PWD}/tools/Sming/Sming

#### rBoot options ####
RBOOT_ENABLED   ?= 1
RBOOT_BIG_FLASH ?= 1
SPI_SIZE        ?= 4M
SPIFF_FILES     ?= spiffs
SPIFF_SIZE      ?= 262144
ESPTOOL2        = ${PWD}/tools/esptool2/esptool2

#### Set the USER_CFLAGS for compilation ####
USER_CFLAGS += "-DCONTROLLER_TYPE=CONTROLLER_TYPE_$(CONTROLLER_TYPE)"
USER_CFLAGS += "-DPLATFORM_TYPE=PLATFORM_TYPE_$(PLATFORM_TYPE)"
USER_CFLAGS += "-DSIGNING_ENABLE=$(MYSENSORS_SIGNING)"
USER_CFLAGS += "-DSIGNING_HMAC=$(MYSENSORS_SIGNING_HMAC)"
USER_CFLAGS += "-DATSHA204I2C=$(MYSENSORS_WITH_ATSHA204)"
USER_CFLAGS += "-DWIRED_ETHERNET_MODE=$(WIRED_ETHERNET_MODE)"
USER_CFLAGS += "-DMEASURE_ENABLE=$(GPIO16_MEASURE_ENABLE)"

# Include main Sming Makefile
ifeq ($(RBOOT_ENABLED), 1)
include $(SMING_HOME)/Makefile-rboot.mk
else
include $(SMING_HOME)/Makefile-project.mk
endif

upload: all
ifndef FTP_SERVER
	$(error FTP_SERVER not set. Please configure it in Makefile-user.mk)
endif
ifndef FTP_USER
	$(error FTP_USER not set. Please configure it in Makefile-user.mk)
endif
ifndef FTP_PASSWORD
	$(error FTP_PASSWORD not set. Please configure it in Makefile-user.mk)
endif
ifndef FTP_PATH
	$(error FTP_PATH not set. Please configure it in Makefile-user.mk)
endif
	@echo "\nUploading firmware..."
	@ncftpput -u $(FTP_USER) -p $(FTP_PASSWORD) $(FTP_SERVER) $(FTP_PATH) out/firmware/*

copy: all
	@sudo rm -f /var/www/firmware/*
	@sudo cp out/firmware/* /var/www/firmware/

