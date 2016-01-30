#ifndef INCLUDE_ICC_H_
#define INCLUDE_ICC_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>
#include <MyMutex.h>

#define FORCE_PUBLISH_DIG_IVL 600
#define FORCE_PUBLISH_ANALOG_IVL 60

typedef Delegate<void(String, String)> I2CChangeDelegate;
extern MyMutex i2cmutex;

class MyI2C
{
  public:
    void begin(I2CChangeDelegate dlg = NULL);
    void setRtcTime(uint32_t ts);
    uint32_t getRtcTime();

  private:
    /* Digital I/O pins */
    void i2cPublishMcpOutputs(byte address, bool forcePublish);
    void i2cPublishMcpInputs(byte address, bool forcePublish);
    void i2cCheckDigitalState();
    
    /* Analog I/O pins */
    void i2cPublishPcfOutputs(byte address, bool forcePublish);
    void i2cPublishPcfInputs(byte address, bool forcePublish);
    void i2cCheckAnalogState();

    /* RTC */
    void i2cCheckRTCState();

    /* OLED */
    void showOLED();

  private:
     
    I2CChangeDelegate changeDlg;

    bool              digitalFound = FALSE;
    bool              analogFound = FALSE;
    bool              lcdFound = FALSE;
    bool              RTCFound = FALSE;
    bool              OLEDFound = FALSE;

    Timer             i2cCheckDigitalTimer;
    Timer             i2cCheckAnalogTimer;
    Timer             i2cCheckRTCTimer;
    Timer             i2cOLEDTimer;

    bool              mcp23017Present[7] = { false, false, false, false,
                                             false, false, false };
    uint8_t           mcp23017Outputs[7] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t           mcp23017Inputs[7] = { 0, 0, 0, 0, 0, 0, 0 };

    bool              pcf8591Present[8] = { false, false, false, false,
                                            false, false, false, false };
    uint8_t           pcf8591Outputs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t           pcf8591Inputs[32] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0 };
};

#endif //INCLUDE_ICC_H_
