#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <Libraries/Adafruit_SSD1306/Adafruit_SSD1306.h>
#include <globals.h>

#include <AppSettings.h>
#include "i2c.h"
#include "mqtt.h"
#include "Network.h"
#include "AppSettings.h"

#include <MySensors/ATSHA204.h>

void MyI2C::begin()
{
    byte error, address;

    Wire.pins(I2C_SCL_PIN, I2C_SDA_PIN); // SCL, SDA
    Wire.begin();

    for (address=0; address < 127; address++)
    {
        Wire.beginTransmission(address);
	error = Wire.endTransmission();

	WDT.alive(); //Make doggy happy

	if (error == 0)
	{
	    if (address >= 0x20 && address <= 0x26)
            {
                Debug.printf("Found MCP23017 expander at %x\n", address);
            }
#if 0
            else if (address == 0x27)
            {
                Debug.printf("Found LCD at address %x\n", address);
                lcd.begin(20, 4);
                lcd.setCursor(0, 0);
                lcd.print((char *)"   cloud-o-mation   ");
                lcd.setCursor(0, 2);
                lcd.print((char *)build_git_sha);
                lcdFound = TRUE;
            }
#endif
            else if (address >= 0x48 && address <= 0x4f)
            {
                Debug.printf("Found PCF8591 expander at %x\n", address);
            }
            else if (address == 0x68)
            {
#if RTC_TYPE == RTC_TYPE_3213
                Debug.printf("Found RTC DS3213 at address %x\n", address);
#elif RTC_TYPE == RTC_TYPE_1307
                Debug.printf("Found RTC DS1307 at address %x\n", address);
#else
    #error "Unknown RTC type"
#endif
            } 
            else if (address == 0x57)
            {
                Debug.printf("Found ATtiny %x\n", address);
            }
            else if (address == 0x64)
            {
                Debug.printf("Found Atsha204 %x\n", address);
#if (ATSHA204I2C)
                ATSHA204Class sha204;
                //sha204.init(); // Be sure to wake up device right as I2C goes up otherwise you'll have NACK issues 
                sha204.dump_configuration();
                // TODO : MUTEX !!! 
                // On my Wemos proto, ATSHA is the only component on the bus.
#endif
            }
            else if (address == 0x3c)
            {
                Debug.printf("Found OLED %x\n", address);
            }
            else
            {
                Debug.printf("Unexpected I2C device found @ %x\n", address);
            }
        }
    }
}
