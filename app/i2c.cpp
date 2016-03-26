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

Adafruit_SSD1306 display(-1); // reset Pin required but later ignored if set to False

void MyI2C::showOLED()
{
	Wire.lock();

	display.clearDisplay();
	// text display tests
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
        display.println("MySensors gateway");
	display.setTextSize(1);
	display.setCursor(0,9);
        if (AppSettings.wired)
        {
            if (!Network.getClientIP().isNull())
            {
                display.print("IP  :");
                display.println(Network.getClientIP().toString());
            } 
            else
            {
                display.setTextColor(BLACK, WHITE); // 'inverted' text
                display.println("connecting ...");
                display.setTextColor(WHITE);
            }
        }
        else
        {
            if (WifiStation.isConnected())
            {
                display.print("AP  :");
                display.println(Network.getClientIP().toString());
            } 
            else
            {
                display.setTextColor(BLACK, WHITE); // 'inverted' text
                display.println("connecting ...");
                display.setTextColor(WHITE);
            }
        }
	display.setCursor(0,18);
        if (isMqttConfigured())
        {
          display.print("MQTT:");
          display.println(MqttServer());
        }
        else
        {
	  display.setTextColor(BLACK, WHITE); // 'inverted' text
          display.println("configure MQTT !");
	  display.setTextColor(WHITE);
        }

	display.setCursor(0,27);
        display.println(SystemClock.getSystemTimeString().c_str());
	display.setCursor(0,36);
        display.print("HEAP :");
	display.setTextColor(BLACK, WHITE); // 'inverted' text
        display.println(system_get_free_heap_size());

	display.setTextColor(WHITE);

	//display.setTextColor(BLACK, WHITE); // 'inverted' text
	//display.setTextSize(3);
	display.display();

	Wire.unlock();
}

void MyI2C::begin(I2CChangeDelegate dlg)
{
    byte error, address;

    changeDlg = dlg;

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
                OLEDFound = TRUE;
                Debug.printf("Found OLED %x\n", address);
                // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)`
                // initialize with the I2C addr 0x3D (for the 128x64)
                // bool:reset set to TRUE or FALSE depending on you display
                display.begin(SSD1306_SWITCHCAPVCC, SSD1306_I2C_ADDRESS, FALSE);
                // display.begin(SSD1306_SWITCHCAPVCC);
                display.display();
            }
            else
            {
                Debug.printf("Unexpected I2C device found @ %x\n", address);
            }
        }
    }

    if (OLEDFound)
    {
        i2cOLEDTimer.initializeMs(1000, TimerDelegate(&MyI2C::showOLED, this)).start(true);
    }
}
