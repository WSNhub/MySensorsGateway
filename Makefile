#####################################################################
#### Please don't change this file. Use Makefile-user.mk instead ####
#####################################################################
# Including user Makefile.
# Should be used to set project-specific parameters
include ./Makefile-user.mk

all: GLOBALS spiff_clean

GLOBALS:
	echo "Generating globals"
	git describe --abbrev=7 --dirty --always --tags | awk ' BEGIN {print "#include \"globals.h\""} {print "const char * build_git_sha = \"" $$0"\";"} END {}' > app/globals.c
	date | awk 'BEGIN {} {print "const char * build_time = \""$$0"\";"} END {} ' >> app/globals.c

#Add your source directories here separated by space
MODULES         = app $(filter %/, $(wildcard libraries/*/))
EXTRA_INCDIR    = include libraries

## ESP_HOME sets the path where ESP tools and SDK are located.
ESP_HOME ?= /opt/esp-open-sdk

## SMING_HOME sets the path where Sming framework is located.
SMING_HOME = ${PWD}/tools/Sming
SMING_HOME = ${PWD}/tools/Sming/Sming

#### rBoot options ####
RBOOT_ENABLED   ?= 1
RBOOT_BIG_FLASH ?= 1
SPI_SIZE        ?= 4M
SPIFF_FILES     ?= spiffs
SPIFF_SIZE      ?= 262144
ESPTOOL2        = ${PWD}/tools/esptool2/esptool2

#### APPLICATION defaults ####
CONTROLLER_TYPE ?= OPENHAB
PLATFORM_TYPE ?= GENERIC

#### Set the USER_CFLAGS for compilation ####
USER_CFLAGS += "-DCONTROLLER_TYPE=CONTROLLER_TYPE_$(CONTROLLER_TYPE)"
USER_CFLAGS += "-DPLATFORM_TYPE=PLATFORM_TYPE_$(PLATFORM_TYPE)"
USER_CFLAGS += "-DSIGNING_ENABLE=$(MYSENSORS_SIGNING)"
USER_CFLAGS += "-DSIGNING_HMAC=$(MYSENSORS_SIGNING_HMAC)"
USER_CFLAGS += "-DATSHA204I2C=$(MYSENSORS_WITH_ATSHA204)"
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

