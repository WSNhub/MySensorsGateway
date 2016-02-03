#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <Libraries/Adafruit_SSD1306/Adafruit_SSD1306.h>

#include <AppSettings.h>
#include "i2c.h"
#include "mqtt.h"
#include "Sodaq_DS3231.h"

#include <SHA204.h>
#include <SHA204Definitions.h>
#include "SHA204I2C.h"

//year, month, date, hour, min, sec and week-day(starts from 0 and goes to 6)
//writing any non-existent time-data may interfere with normal operation of the RTC.
//Take care of week-day also.
static char weekDay[][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
MyDateTime dt(2016, 1, 22, 17, 34, 0, 6);
Adafruit_SSD1306 display(-1); // reset Pin required but later ignored if set to False
MyMutex i2cmutex;


void MyI2C::showOLED()
{
	i2cmutex.Lock();

	display.clearDisplay();
	// text display tests
	display.setTextSize(1);
	display.setTextColor(WHITE);
	display.setCursor(0,0);
        display.println("MySensors gateway");
	display.setTextSize(1);
	display.setCursor(0,9);
        if (WifiStation.isConnected())
        {
          display.print("AP  :");
          display.println(WifiStation.getIP().toString());
        } 
        else
        {
	  display.setTextColor(BLACK, WHITE); // 'inverted' text
          display.println("connecting ...");
	  display.setTextColor(WHITE);
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

	i2cmutex.Unlock();
}

void MyI2C::i2cPublishMcpOutputs(byte address, bool forcePublish)
{
    uint8_t outputs;

    /* read the actual value */
    Wire.beginTransmission(address);
    Wire.write(0x12); //port A
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    outputs=Wire.read();

    /* apply the inversions and publish */
    uint8_t outputInvert = 0; //TODO AppSettings.getMcpOutputInvert(address - 0x20);
    for (int j = 0; j < 8; j++)
    {
        if (outputInvert & (1 << j))
        {
            if (outputs & (1 << j))
            {
                //The bit is set, clear it
                outputs &= ~(1 << j);
            }
            else
            {
                //The bit is not set, set it
                outputs |= (1 << j);
            }
        }

        if (forcePublish ||
            (mcp23017Outputs[address - 0x20] & (1 << j)) != (outputs & (1 << j)))
        {
            if (changeDlg)
                changeDlg(String("output-d") +
                          String((j + 1) + (8 * (address - 0x20))),
                          outputs & (1 << j) ? "on" : "off");
        }
    }

    mcp23017Outputs[address - 0x20] = outputs;
}

void MyI2C::i2cPublishMcpInputs(byte address, bool forcePublish)
{
    uint8_t inputs;

    /* read the actual value */
    Wire.beginTransmission(address);
    Wire.write(0x13); //port B
    Wire.endTransmission();
    Wire.requestFrom(address, 1);
    inputs=Wire.read();

    for (int j = 0; j < 8; j++)
    {
        if (forcePublish ||
            (mcp23017Inputs[address - 0x20] & (1 << j)) != (inputs & (1 << j)))
        {
            if (changeDlg)
                changeDlg(String("input-d") +
                          String((j + 1) + (8 * (address - 0x20))),
                          inputs & (1 << j) ? "on" : "off");
        }
    }

    mcp23017Inputs[address - 0x20] = inputs;
}

void MyI2C::i2cCheckDigitalState()
{
    static int forcePublish = FORCE_PUBLISH_DIG_IVL;

    i2cmutex.Lock();

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
        forcePublish = FORCE_PUBLISH_DIG_IVL;

    i2cmutex.Unlock();
}

void MyI2C::i2cPublishPcfOutputs(byte address, bool forcePublish)
{
    /* The value can not be read back */
    /*
    if (forcePublish)
    {
        if (changeDlg)
                changeDlg(String("outputs/a") +
                          String(1 + (address - 0x48)),
                          String(pcf8591Outputs[address - 0x48]));
    }
    */
}

void MyI2C::i2cPublishPcfInputs(byte address, bool forcePublish)
{
    byte value[4];

    /* read the actual values */
    Wire.beginTransmission(address); // wake up PCF8591
    Wire.write(0x04); // control byte - read ADC0 then auto-increment
    Wire.endTransmission(); // end tranmission
    Wire.requestFrom(address, 5);
    value[0]=Wire.read();
    value[0]=Wire.read();
    value[1]=Wire.read();
    value[2]=Wire.read();
    value[3]=Wire.read();

    for (int j = 0; j < 4; j++)
    {
        if (forcePublish ||
            pcf8591Inputs[j + (4 * (address - 0x48))] != value[j])
        {
            if (changeDlg)
                changeDlg(String("input-a") +
                          String((j + 1) + (4 * (address - 0x48))),
                          String(value[j]));
        }
        pcf8591Inputs[j + (4 * (address - 0x48))] = value[j];
    }
}

void MyI2C::i2cCheckAnalogState()
{
    static int forcePublish = FORCE_PUBLISH_ANALOG_IVL;

    i2cmutex.Lock();

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
        forcePublish = FORCE_PUBLISH_ANALOG_IVL;

    i2cmutex.Unlock();
}

void MyI2C::i2cCheckRTCState()
{
    //MyDateTime now = rtc.now(); //get the current date-time from RTC
    //Debug.printf("From RTC\n");
    //Debug.printf("%d-%d-%d\n",now.date(),now.month(),now.year());
    //Debug.printf("%d:%d:%d\n",now.hour(),now.minute(),now.second());
    //Debug.printf("day of week : %s\n",weekDay[now.DdayOfWeek()]);
    SystemClock.setTime(rtc.now().getEpoch(), eTZ_UTC);

    rtc.convertTemperature();             //convert current temperature into registers
    Debug.printf("Plug temperature %02f deg C\n", rtc.getTemperature()); //read registers and display the temperature
    if (changeDlg)
        changeDlg("RTC-temperature", String(rtc.getTemperature()));
}

void MyI2C::begin(I2CChangeDelegate dlg)
{
    byte error, address;

    changeDlg = dlg;

    // Belows works on Wemos with swapped ATSHA204 pinning as in datasheet !
    Wire.pins(4, 5); // needed to swap : SCL, SDA  : will fix PCB !!!!
    // Belows works on Wemos with ATSHA204 pinning as in datasheet
    //Wire.pins(5, 4); // needed to swap : SCL, SDA  : will fix PCB !!!!
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
                SystemClock.setTimeZone(1); //TODO make this configurable!
                SystemClock.setTime(rtc.now().getEpoch(), eTZ_UTC);
                Debug.print(" Time = ");
                Debug.println(SystemClock.getSystemTimeString());
     
                //rtc.setMyDateTime(dt); //one time Adjust date-time as defined 'dt' above 
                //MyDateTime now = rtc.now(); //get the current date-time from RTC
                //Debug.printf("From RTC\n");
                //Debug.printf("year %d\n",now.year());
                //Debug.printf("month %d\n",now.month());
                //Debug.printf("day %d\n",now.date());
                //Debug.printf("hour %d\n",now.hour());
                //Debug.printf("minute %d\n",now.minute());
                //Debug.printf("second %d\n",now.second());
                //Debug.printf("day of week %s\n",weekDay[now.DdayOfWeek()]);

                rtc.convertTemperature();             //convert current temperature into registers
                Debug.printf(" %02f deg C\n", rtc.getTemperature()); //read registers and display the temperature
            } 
            else if (address == 0x57)
            {
                Debug.printf("Found ATtiny %x\n", address);
            }
            else if (address == 0x64)
            {
                Debug.printf("Found Atsha204 %x\n", address);
                uint8_t response[SHA204_RSP_SIZE_MIN];
                byte returnValue;
                SHA204I2C sha204dev;
                uint8_t serialNumber[9];
                // TODO : MUTEX !!! 
                // On my Wemos proto, ATSHA is the only component on the bus.

                sha204dev.init(); // Be sure to wake up device right as I2C goes up otherwise you'll have NACK issues 

                //BEGIN of TESTCODE
                returnValue = sha204dev.serialNumber(serialNumber);
                for (int i=0; i<9; i++) {
                    Debug.print(serialNumber[i], HEX);
                    Debug.print(" ");
                }
                Debug.println();
                Debug.println("Asking the SHA204's serial number. Response should be:");
                Debug.println("1 23 x x x x x x x EE");
                Debug.println("-------"); 

                returnValue = sha204dev.resync(4, &response[0]);
                for (int i=0; i<SHA204_RSP_SIZE_MIN; i++) {
                    Debug.print(response[i], HEX);
                    Debug.print(" ");
                }
                Debug.println();
                //END of TESTCODE

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
        i2cCheckRTCTimer.initializeMs(60000, TimerDelegate(&MyI2C::i2cCheckRTCState, this)).start(true);
    }

    if (OLEDFound)
    {
        i2cOLEDTimer.initializeMs(1000, TimerDelegate(&MyI2C::showOLED, this)).start(true);
    }

    if (!digitalFound && !analogFound && !lcdFound && !RTCFound)
    {
        Debug.printf("No I2C devices found\n");
    }
 
}

void MyI2C::setRtcTime(uint32_t ts)
{
    if (RTCFound)
        rtc.setEpoch(ts);
}

uint32_t MyI2C::getRtcTime()
{
    if (RTCFound)
        return rtc.now().getEpoch();
    return 0;
}
