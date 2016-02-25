#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>
#include <AppSettings.h>
#include "IOExpansion.h"

bool IOExpansion::i2cSetMcpOutput(uint8_t output, bool enable)
{
    uint8_t port;
    uint8_t pin;
    uint8_t outputs;
    
    /*
     * Output is any number between 1 and 56, boundaries included
     * so the port and pin number have to be derived from it.
     */

    if (output < 1 || output > 56)
    {
        Debug.printf("invalid output %d!!!!\n", output);
        return false;
    }

    port = (output - 1) / 8;
    pin = output - 1 - (port * 8);
    Debug.printf("Set output %d %s [port=%d pin=%d]\n",
                  output, enable ? "on" : "off", port, pin);

    /* read the actual value */
    Wire.beginTransmission(0x20 + port);
    Wire.write(0x12); //port A
    Wire.endTransmission();
    Wire.requestFrom(0x20 + port, 1);
    outputs=Wire.read();

//    if (AppSettings.getMcpOutputInvert(port) & (1 << pin))
//        enable = !enable;

    if (!enable)
        outputs &= ~(1 << pin);
    else
        outputs |= (1 << pin);

    Wire.beginTransmission(0x20 + port);
    Wire.write(0x12); // address port A
    Wire.write(outputs);  // value to send
    Wire.endTransmission();

    i2cPublishMcpOutputs(0x20 + port, false /* forcePublish */);
    return true;
}

bool IOExpansion::i2cToggleMcpOutput(uint8_t output)
{
    uint8_t port;
    uint8_t pin;
    uint8_t outputs;
    
    /*
     * Output is any number between 1 and 56, boundaries included
     * so the port and pin number have to be derived from it.
     */

    if (output < 1 || output > 56)
    {
        Debug.printf("invalid output %d!!!!\n", output);
        return false;
    }

    port = (output - 1) / 8;
    pin = output - 1 - (port * 8);
    Debug.printf("Toggle output %d [port=%d pin=%d]\n",
                  output, port, pin);

    /* read the actual value */
    Wire.beginTransmission(0x20 + port);
    Wire.write(0x12); //port A
    Wire.endTransmission();
    Wire.requestFrom(0x20 + port, 1);
    outputs=Wire.read();

    bool newState = outputs & (1 << pin) ? false : true;

    if (!newState)
        outputs &= ~(1 << pin);
    else
        outputs |= (1 << pin);

    Wire.beginTransmission(0x20 + port);
    Wire.write(0x12); // address port A
    Wire.write(outputs);  // value to send
    Wire.endTransmission();

    i2cPublishMcpOutputs(0x20 + port, false /* forcePublish */);
    return true;
}

bool IOExpansion::i2cSetMcpOutputInvert(uint8_t output, bool invert)
{
#if 0
    uint8_t port;
    uint8_t pin;
    uint8_t invertMask;
    
    /*
     * Output is any number between 1 and 56, boundaries included
     * so the port and pin number have to be derived from it.
     */

    if (output < 1 || output > 56)
    {
        Debug.printf("invalid output %d!!!!\n", output);
        return false;
    }

    port = (output - 1) / 8;
    pin = output - 1 - (port * 8);
    Debug.printf("Set invert output %d %s [port=%d pin=%d]\n",
                  output, invert ? "yes" : "no", port, pin);

    invertMask = AppSettings.getMcpOutputInvert(port);
    if (!invert)
        invertMask &= ~(1 << pin);
    else
        invertMask |= (1 << pin);

    AppSettings.setMcpOutputInvert(port, invertMask);
    AppSettings.save();
    
    i2cPublishMcpOutputs(0x20 + port, false /* forcePublish */);
#endif
    return true;
}

bool IOExpansion::i2cSetMcpInputInvert(uint8_t input, bool invert)
{
#if 0
    uint8_t port;
    uint8_t pin;
    uint8_t invertMask;
    
    /*
     * Output is any number between 1 and 56, boundaries included
     * so the port and pin number have to be derived from it.
     */

    if (input < 1 || input > 56)
    {
        Debug.printf("invalid output %d!!!!\n", input);
        return false;
    }

    port = (input - 1) / 8;
    pin = input - 1 - (port * 8);
    Debug.printf("Set invert input %d %s [port=%d pin=%d]\n",
                  input, invert ? "yes" : "no", port, pin);

    invertMask = AppSettings.getMcpInputInvert(port);
    if (!invert)
        invertMask &= ~(1 << pin);
    else
        invertMask |= (1 << pin);

    AppSettings.setMcpInputInvert(port, invertMask);
    AppSettings.save();
    
    Wire.beginTransmission(0x20 + port);
    Wire.write(0x03); // input invert register
    Wire.write(invertMask);

    i2cPublishMcpInputs(0x20 + port, false /* forcePublish */);
#endif
    return true;
}

void IOExpansion::i2cPublishMcpOutputs(byte address, bool forcePublish)
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
                changeDlg(String("outputD") +
                          String((j + 1) + (8 * (address - 0x20))),
                          outputs & (1 << j) ? "on" : "off");
        }
    }

    mcp23017Outputs[address - 0x20] = outputs;
}

void IOExpansion::i2cPublishMcpInputs(byte address, bool forcePublish)
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
                changeDlg(String("inputD") +
                          String((j + 1) + (8 * (address - 0x20))),
                          inputs & (1 << j) ? "on" : "off");
        }
    }

    mcp23017Inputs[address - 0x20] = inputs;
}

void IOExpansion::i2cCheckDigitalState()
{
    static int forcePublish = FORCE_PUBLISH_DIG_IVL;

    Wire.lock();

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

    Wire.unlock();
}

void IOExpansion::i2cPublishPcfOutputs(byte address, bool forcePublish)
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

void IOExpansion::i2cPublishPcfInputs(byte address, bool forcePublish)
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
                changeDlg(String("inputA") +
                          String((j + 1) + (4 * (address - 0x48))),
                          String(value[j]));
        }
        pcf8591Inputs[j + (4 * (address - 0x48))] = value[j];
    }
}

void IOExpansion::i2cCheckAnalogState()
{
    static int forcePublish = FORCE_PUBLISH_ANALOG_IVL;

    Wire.lock();

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

    Wire.unlock();
}

void IOExpansion::begin(IOChangeDelegate dlg)
{
    byte error, address;

    changeDlg = dlg;

    for (address = 0x20; address <= 0x26; address++)
    {
        Wire.lock();

        Wire.beginTransmission(address);
	error = Wire.endTransmission();

	WDT.alive(); //Make doggy happy

	if (error == 0)
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

        Wire.unlock();
    }

    for (address = 0x48; address <= 0x4f; address++)
    {
        Wire.lock();

        Wire.beginTransmission(address);
	error = Wire.endTransmission();

	WDT.alive(); //Make doggy happy

	if (error == 0)
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

        Wire.unlock();
    }

    if (digitalFound)
    {
        i2cCheckDigitalTimer.initializeMs(100, TimerDelegate(&IOExpansion::i2cCheckDigitalState, this)).start(true);
    }

    if (analogFound)
    {
        i2cCheckAnalogTimer.initializeMs(10000, TimerDelegate(&IOExpansion::i2cCheckAnalogState, this)).start(true);
    }

    if (!digitalFound && !analogFound)
    {
        Debug.printf("No I/O expanders found\n");
    }
}

bool IOExpansion::updateResource(String resource, String value)
{
    if (resource.startsWith("outputD"))
    {
        resource = resource.substring(6);
        int out = resource.toInt();
        if (out < 1 || out > 56)
        {
            Debug.printf("invalid output: d%d", out);
            return false;
        }

        Debug.printf("Set digital output: [%d] %s\n", out, value.c_str());
        i2cSetMcpOutput(out, value.equals("on"));
    }
    else
    {
        Debug.println("ERROR: Only digital outputs can be set");
        return false;
    }

    return true;
}

String IOExpansion::getResourceValue(String resource)
{
    uint8_t port;
    uint8_t pin;

    /* Digital input */
    if (resource.startsWith("inputD"))
    {
        resource = resource.substring(6);
        int input = resource.toInt();
        if (input < 1 || input > 56)
        {
            Debug.printf("invalid input: d%d", input);
            return "invalid";
        }

        port = (input - 1) / 8;
        pin = input - 1 - (port * 8);
        Debug.printf("Get input D%d [port=%d pin=%d]\n",
                     input, port, pin);
        return (mcp23017Inputs[port] & (1 << pin)) ? "on" : "off";
    }
    /* Digital input */
    if (resource.startsWith("inputA"))
    {
        resource = resource.substring(6);
        int input = resource.toInt();
        if (input < 1 || input > 32)
        {
            Debug.printf("invalid input: d%d", input);
            return "invalid";
        }

        Debug.printf("Get input A%d\n", input);
        return String(pcf8591Inputs[input - 1]);
    }
    /* Digital output */
    else if (resource.startsWith("outputD"))
    {
        resource = resource.substring(7);
        int output = resource.toInt();
        if (output < 1 || output > 56)
        {
            Debug.printf("invalid output: d%d", output);
            return "invalid";
        }

        port = (output - 1) / 8;
        pin = output - 1 - (port * 8);
        Debug.printf("Get output %d [port=%d pin=%d]\n",
                     output, port, pin);
        return (mcp23017Outputs[port] & (1 << pin)) ? "on" : "off";
    }

    Debug.println("ERROR: GetValue for an unknown object");
    return "invalid";
}

IOExpansion Expansion;
