#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>
#include "i2c.h"
#include "Sodaq_DS3231.h"

//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
static char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
MyDateTime dt(2016, 1, 22, 17, 34, 0, 6);


void MyI2C::i2cCheckDigitalState()
{
/*    #define FORCE_PUBLISH_DIG_IVL 600
    static int forcePublish = FORCE_PUBLISH_DIG_IVL; //every 60 seconds

    forcePublish--;
    for (int i = 0; i < 7; i++)
    {
        if (mcp23017Present[i])
        {
            i2cPublishMcpInputs(0x20 + i, (forcePublish == 0));
            if (forcePublish == 0)
                i2cPublishMcpOutputs(0x20 + i, true);
        }
    }

    if (forcePublish == 0)
        forcePublish = FORCE_PUBLISH_DIG_IVL;*/
}

void MyI2C::i2cCheckAnalogState()
{
/*    #define FORCE_PUBLISH_ANALOG_IVL 60
    static int forcePublish = FORCE_PUBLISH_ANALOG_IVL; //every 60 seconds

    forcePublish--;
    for (int i = 0; i < 8; i++)
    {
        if (pcf8591Present[i])
        {
            i2cPublishPcfInputs(0x48 + i, (forcePublish == 0));
            if (forcePublish == 0)
                i2cPublishPcfOutputs(0x48 + i, true);
        }
    }

    if (forcePublish == 0)
        forcePublish = FORCE_PUBLISH_ANALOG_IVL;*/
}

void MyI2C::i2cCheckRTCState()
{
  MyDateTime now = rtc.now(); //get the current date-time from RTC
  Debug.printf("From RTC\n");
  Debug.printf("%d-%d-%d\n",now.date(),now.month(),now.year());
  Debug.printf("%d:%d:%d\n",now.hour(),now.minute(),now.second());
  Debug.printf("day of week : %s\n",weekDay[now.DdayOfWeek()]);

  rtc.convertTemperature();             //convert current temperature into registers
  Debug.printf("Plug temperature %02f deg C\n", rtc.getTemperature()); //read registers and display the temperature
}


void MyI2C::begin()
{
    bool digitalFound = FALSE;
    bool analogFound = FALSE;
    bool lcdFound = FALSE;
    bool RTCFound = FALSE;
    byte error, address;

    Wire.pins(4, 5); // needed to swap : SCL, SDA  : will fix PCB !!!!
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
                uint8_t data;
                digitalFound = TRUE;

                /*
                 * MCP23017 16-bit port expanders
                 *
                 * 7 of these are supported, with addresses from 0x20 to 0x26.
                 * These are configured so that port A are all outputs and
                 * port B are all inputs.
                 */
                Debug.printf("Found MCP23017 expander at %x\n", address);
                mcp23017Present[address - 0x20] = true;

                /* set all of port A to outputs */
                Wire.beginTransmission(address);
                Wire.write(0x00); // IODIRA register
                Wire.write(0x00); // set all of port A to outputs
                Wire.endTransmission();

#if 0
                /* if inversion is configured, set outputs to the correct state */
                uint8_t outputInvert =
                    AppSettings.getMcpOutputInvert(address - 0x20);
                if (outputInvert)
                {
                    Wire.beginTransmission(address);
                    Wire.write(0x12); // address port A
                    Wire.write(outputInvert);  // value to send
                    Wire.endTransmission();
                }
#endif

                //set all of port B to inputs
                Wire.beginTransmission(address);
                Wire.write(0x01); // IODIRB register
                Wire.write(0xff); // set all of port A to outputs
                Wire.endTransmission();
#if 0
                Wire.beginTransmission(address);
                Wire.write(0x03); // input invert register
                Wire.write(AppSettings.getMcpInputInvert(address - 0x20));
                Wire.endTransmission();
#endif
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
                /*
                 * PCF8591 D/A and A/D expanders
                 *
                 * 8 of these are supported, with addresses from 0x48 to 0x4f.
                 * These are configured so that port A are all outputs and
                 * port B are all inputs.
                 */
                Debug.printf("Found PCF8591 expander at %x\n", address);
                pcf8591Present[address - 0x48] = true;
                analogFound = TRUE;

                /* The output value can not be read back, so init to 0. */
                Wire.beginTransmission(address); // wake up PCF8591
                Wire.write(0x40); // control byte - turn on DAC (binary 1000000)
                Wire.write(0); // value to send to DAC
                Wire.endTransmission(); // end tranmission
                pcf8591Outputs[address - 0x48] = 0;
            }
            else if (address == 0x68)
            {
                struct tm   time,stime;
                bool  flag;

                RTCFound = TRUE;
                Debug.printf("Found RTC DS3213 at address %x\n", address);
                rtc.begin(address);
                //rtc.setMyDateTime(dt); //one time Adjust date-time as defined 'dt' above 
                MyDateTime now = rtc.now(); //get the current date-time from RTC
                Debug.printf("From RTC\n");
                Debug.printf("year %d\n",now.year());
                Debug.printf("month %d\n",now.month());
                Debug.printf("day %d\n",now.date());
                Debug.printf("hour %d\n",now.hour());
                Debug.printf("minute %d\n",now.minute());
                Debug.printf("second %d\n",now.second());
                Debug.printf("day of week %s\n",weekDay[now.DdayOfWeek()]);
                rtc.convertTemperature();             //convert current temperature into registers
                Debug.printf(" %02f deg C\n", rtc.getTemperature()); //read registers and display the temperature
            } 
            else if (address == 0x57)
            {
                Debug.printf("Found ATtiny %x\n", address);
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

    if (digitalFound)
    {
        i2cCheckDigitalTimer.initializeMs(100, TimerDelegate(&MyI2C::i2cCheckDigitalState, this)).start(true);
    }

    if (analogFound)
    {
        i2cCheckAnalogTimer.initializeMs(10000, TimerDelegate(&MyI2C::i2cCheckAnalogState, this)).start(true);
    }

    if (RTCFound)
    {
        i2cCheckRTCTimer.initializeMs(10000, TimerDelegate(&MyI2C::i2cCheckRTCState, this)).start(true);
    }


    if (!digitalFound && !analogFound && !lcdFound && !RTCFound)
    {
        Debug.printf("No I2C devices found\n");
    }
 
}

