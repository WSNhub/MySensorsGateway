#ifndef INCLUDE_IOEXPANSION_H_
#define INCLUDE_IOEXPANSION_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

#define FORCE_PUBLISH_DIG_IVL 600
#define FORCE_PUBLISH_ANALOG_IVL 60

typedef Delegate<void(String, String)> IOChangeDelegate;

class IOExpansion
{
  public:
    void begin(IOChangeDelegate dlg = NULL);
    bool updateResource(String resource, String value);
    String getResourceValue(String resource);

  private:
    /* Digital I/O pins */
    bool i2cSetMcpOutput(uint8_t output, bool enable);
    bool i2cToggleMcpOutput(uint8_t output);
    bool i2cSetMcpOutputInvert(uint8_t output, bool invert);
    bool i2cSetMcpInputInvert(uint8_t input, bool invert);
    void i2cPublishMcpOutputs(byte address, bool forcePublish);
    void i2cPublishMcpInputs(byte address, bool forcePublish);
    void i2cCheckDigitalState();
    
    /* Analog I/O pins */
    void i2cPublishPcfOutputs(byte address, bool forcePublish);
    void i2cPublishPcfInputs(byte address, bool forcePublish);
    void i2cCheckAnalogState();

  private:
     
    IOChangeDelegate  changeDlg;

    bool              digitalFound = FALSE;
    bool              analogFound = FALSE;

    Timer             i2cCheckDigitalTimer;
    Timer             i2cCheckAnalogTimer;

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

extern IOExpansion Expansion;

#endif //INCLUDE_IO_H_
