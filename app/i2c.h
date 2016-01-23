#ifndef INCLUDE_ICC_H_
#define INCLUDE_ICC_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

class MyI2C
{
  public:
    void begin();

  private:
    void i2cCheckDigitalState();
    void i2cCheckAnalogState();
    void i2cCheckRTCState();

  private:
    Timer            i2cCheckDigitalTimer;
    Timer            i2cCheckAnalogTimer;
    Timer            i2cCheckRTCTimer;

    bool             mcp23017Present[7] = { false, false, false, false,
                                            false, false, false };
    uint8_t          mcp23017Outputs[7] = { 0, 0, 0, 0, 0, 0, 0 };
    uint8_t          mcp23017Inputs[7] = { 0, 0, 0, 0, 0, 0, 0 };

    bool             pcf8591Present[8] = { false, false, false, false,
                                           false, false, false, false };
    uint8_t          pcf8591Outputs[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t          pcf8591Inputs[32] = { 0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0,
                                           0, 0, 0, 0, 0, 0, 0, 0 };
};

#endif //INCLUDE_ICC_H_
