#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>
#include <AppSettings.h>
#include "IOExpansion.h"

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
                changeDlg(String("output-d") +
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
                changeDlg(String("input-d") +
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
                changeDlg(String("input-a") +
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
