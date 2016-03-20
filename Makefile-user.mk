#### Set the controller type ####
# For now the only sane option is OPENHAB
# Alternative is CLOUD but not generally available
CONTROLLER_TYPE = OPENHAB

#### Set the platform type ####
# For now the choice is either GENERIC or SDSHIELD
PLATFORM_TYPE = GENERIC

#### Enable or disable signing ####
# Either 0 to disable or 1 to enable
MYSENSORS_SIGNING = 0
MYSENSORS_WITH_ATSHA204 = 0
MYSENSORS_SIGNING_HMAC = { 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08, \
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
WIRED_ETHERNET_MODE = WIRED_ETHERNET_NONE

#### Enable automatic upgrades of the SMING library ####
# A reason to disable this would be if your computer does
# not have an internet connection all the time.
# Does that still exist?
SMING_AUTO_UPGRADE = 1

GPIO16_MEASURE_ENABLE = 0

#### Set CFLAGS if needed ####
USER_CFLAGS = 
