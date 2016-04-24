#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>
#include <AppSettings.h>
#include "IOExpansion.h"

#define MCP23017_ADDRESS 0x20

#define MCP23017_IODIRA 0x00
#define MCP23017_IPOLA 0x02
#define MCP23017_GPINTENA 0x04
#define MCP23017_DEFVALA 0x06
#define MCP23017_INTCONA 0x08
#define MCP23017_IOCONA 0x0A
#define MCP23017_GPPUA 0x0C
#define MCP23017_INTFA 0x0E
#define MCP23017_INTCAPA 0x10
#define MCP23017_GPIOA 0x12
#define MCP23017_OLATA 0x14


#define MCP23017_IODIRB 0x01
#define MCP23017_IPOLB 0x03
#define MCP23017_GPINTENB 0x05
#define MCP23017_DEFVALB 0x07
#define MCP23017_INTCONB 0x09
#define MCP23017_IOCONB 0x0B
#define MCP23017_GPPUB 0x0D
#define MCP23017_INTFB 0x0F
#define MCP23017_INTCAPB 0x11
#define MCP23017_GPIOB 0x13
#define MCP23017_OLATB 0x15

#define MCP23017_INT_ERR 255

struct DigitalPin
{
    uint8_t id;
    uint8_t i2caddr;
    uint8_t pin;
    bool    enabled;

    uint8_t bitForPin(uint8_t pin)
    {
        return pin%8;
    }

    uint8_t regForPin(uint8_t pin, uint8_t portAaddr, uint8_t portBaddr)
    {
        return (pin<8) ? portAaddr : portBaddr;
    }

    uint8_t readRegister(uint8_t addr)
    {
        // read the current GPINTEN
        Wire.beginTransmission(MCP23017_ADDRESS | i2caddr);
        Wire.write((uint8_t)addr);
        Wire.endTransmission();
        Wire.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
        return Wire.read();
    }

    void writeRegister(uint8_t regAddr, uint8_t regValue)
    {
        // Write the register
        Wire.beginTransmission(MCP23017_ADDRESS | i2caddr);
        Wire.write((uint8_t)regAddr);
        Wire.write((uint8_t)regValue);
        Wire.endTransmission();
    }

    void updateRegisterBit(uint8_t pin, uint8_t pValue,
                           uint8_t portAaddr, uint8_t portBaddr)
    {
        uint8_t regValue;
        uint8_t regAddr=regForPin(pin,portAaddr,portBaddr);
        uint8_t bit=bitForPin(pin);
        regValue = readRegister(regAddr);

        // set the value for the particular bit
        bitWrite(regValue,bit,pValue);

        writeRegister(regAddr,regValue);
    }

    void pinMode(uint8_t d)
    {
        updateRegisterBit(pin,(d==INPUT),MCP23017_IODIRA,MCP23017_IODIRB);
    }

    uint8_t readGPIO(uint8_t b)
    {
        // read the current GPIO output latches
        Wire.beginTransmission(MCP23017_ADDRESS | i2caddr);
        if (b == 0)
            Wire.write((uint8_t)MCP23017_GPIOA);
        else
        {
            Wire.write((uint8_t)MCP23017_GPIOB);
        }
        Wire.endTransmission();

        Wire.requestFrom(MCP23017_ADDRESS | i2caddr, 1);
        return Wire.read();
    }

    void digitalWrite(uint8_t d)
    {
        uint8_t gpio;
        uint8_t bit=bitForPin(pin);

        // read the current GPIO output latches
        uint8_t regAddr=regForPin(pin,MCP23017_OLATA,MCP23017_OLATB);
        gpio = readRegister(regAddr);

        // set the pin and direction
        bitWrite(gpio,bit,d);

        // write the new GPIO
        regAddr=regForPin(pin,MCP23017_GPIOA,MCP23017_GPIOB);
        writeRegister(regAddr,gpio);
    }

    void pullUp(uint8_t d)
    {
        updateRegisterBit(pin,d,MCP23017_GPPUA,MCP23017_GPPUB);
    }

    uint8_t digitalRead()
    {
        uint8_t bit=bitForPin(pin);
        uint8_t regAddr=regForPin(pin,MCP23017_GPIOA,MCP23017_GPIOB);
        return (readRegister(regAddr) >> bit) & 0x1;
    }

    bool checkState()
    {
        bool newEnabled = digitalRead() > 0 ? true : false;
        if (newEnabled != enabled)
        {
            enabled = newEnabled;
            return true;
        }
        return false;
    }
};

DigitalPin DigitalOutputPins[] =
{
    { 1, 0x20, 0}, // expander 0x20 pin 0
    { 2, 0x20, 1}, // expander 0x20 pin 1
    { 3, 0x20, 2}, // expander 0x20 pin 2
    { 4, 0x20, 3}, // expander 0x20 pin 3
    { 5, 0x20, 4}, // expander 0x20 pin 4
    { 6, 0x20, 5}, // expander 0x20 pin 5
    { 7, 0x20, 6}, // expander 0x20 pin 6
    { 8, 0x20, 7}, // expander 0x20 pin 7
};

DigitalPin DigitalInputPins[] =
{
    { 1, 0x20, 13}, // expander 0x20 pin 0
    { 2, 0x20, 9},  // expander 0x20 pin 0
    { 3, 0x20, 10}, // expander 0x20 pin 0
    { 4, 0x20, 11}, // expander 0x20 pin 0
    { 5, 0x20, 12}, // expander 0x20 pin 0
    { 6, 0x20, 8},  // expander 0x20 pin 0
    { 7, 0x20, 14}, // expander 0x20 pin 0
    { 8, 0x20, 15}, // expander 0x20 pin 0
};

void IOExpansion::begin(IOChangeDelegate dlg)
{
    byte error, address;
    uint8_t input = 0;
    uint8_t output = 0;
    changeDlg = dlg;

    /*
     * MCP23017 16-bit port expanders
     * 7 of these are supported, with addresses from 0x20 to 0x26.
     */
    for (address = 0x20; address <= 0x26; address++)
    {
        uint8_t data;

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        /* Lock the I2C bus */
        Wire.lock();

        /* Check whether the device responds, if not it's not present */
        Wire.beginTransmission(address);
	error = Wire.endTransmission();

	if (error != 0)
	{
            Wire.unlock();
            continue;
        }

        digitalFound = TRUE;

        Debug.printf("Found MCP23017 expander at %x\n", address);
        mcp23017Present[address - 0x20] = true;

        /* Unlock the I2C bus */
        Wire.unlock();        
    }

    for (int i=0; i<(sizeof(DigitalInputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalInputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->i2caddr == 0)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            continue;

        Wire.lock();
        pPin->pinMode(INPUT);
        pPin->checkState();
        Wire.unlock();
    }
    
    for (int i=0; i<(sizeof(DigitalOutputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalOutputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->i2caddr == 0)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            continue;

        Wire.lock();
        pPin->pinMode(OUTPUT);
        pPin->checkState();
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

void IOExpansion::i2cCheckDigitalState()
{
    static int forcePublish = FORCE_PUBLISH_DIG_IVL;

    forcePublish--;

    for (int i=0; i<(sizeof(DigitalInputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalInputPins[i];
        bool changed;
        bool enabled;

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->i2caddr == 0)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            continue;

        Wire.lock();
        changed = pPin->checkState();
        enabled = pPin->enabled;
        Wire.unlock();
        
        if (changed)
        {
            Debug.printf("inputD%d => %s\n", pPin->id,
                         enabled ? "on" : "off");

            if (changeDlg)
                changeDlg(String("inputD") + String(pPin->id),
                          enabled ? "on" : "off");
        }
    }

    for (int i=0; i<(sizeof(DigitalOutputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalOutputPins[i];
        bool changed;
        bool enabled;

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->i2caddr == 0)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            continue;

        Wire.lock();
        changed = pPin->checkState();
        enabled = pPin->enabled;
        Wire.unlock();

        if (changed)
        {
            Debug.printf("outputD%d => %s\n",  pPin->id,
                         enabled ? "on" : "off");

            if (changeDlg)
                changeDlg(String("outputD") + String(pPin->id),
                          enabled ? "on" : "off");
        }
    }

    if (forcePublish == 0)
        forcePublish = FORCE_PUBLISH_DIG_IVL;
}

bool IOExpansion::getDigOutput(uint8_t output)
{
    bool enabled = false;

    for (int i=0; i<(sizeof(DigitalOutputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalOutputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->id != output)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            break;

        Wire.lock();
        enabled = pPin->enabled;
        Wire.unlock();

        return enabled;
    }

    Debug.printf("invalid output %d!!!!\n", output);
    return false;
}

bool IOExpansion::setDigOutput(uint8_t output, bool enable)
{
    bool success = false;

    for (int i=0; i<(sizeof(DigitalOutputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalOutputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->id != output)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            break;

        Wire.lock();
        pPin->digitalWrite(enable ? 1 : 0);
        Wire.unlock();

        Debug.printf("outputD%d => %s\n",  pPin->id,
                     pPin->enabled ? "on" : "off");
        if (changeDlg)
            changeDlg(String("outputD") + String(pPin->id),
                      pPin->enabled ? "on" : "off");

        success = true;
        break;
    }

    if (!success)
    {
        Debug.printf("invalid output %d!!!!\n", output);
        return false;
    }

    return true;
}

bool IOExpansion::toggleDigOutput(uint8_t output)
{
    bool success = false;

    for (int i=0; i<(sizeof(DigitalOutputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalOutputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->id != output)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            break;

        Wire.lock();
        pPin->digitalWrite(pPin->enabled ? 0 : 1);
        Wire.unlock();

        Debug.printf("outputD%d => %s\n",  pPin->id,
                     pPin->enabled ? "on" : "off");
        if (changeDlg)
            changeDlg(String("outputD") + String(pPin->id),
                      pPin->enabled ? "on" : "off");

        success = true;
        break;
    }

    if (!success)
    {
        Debug.printf("invalid output %d!!!!\n", output);
        return false;
    }

    return true;
}

bool IOExpansion::getDigInput(uint8_t input)
{
    bool enabled = false;

    for (int i=0; i<(sizeof(DigitalInputPins)/sizeof(DigitalPin)); i++)
    {
        DigitalPin *pPin = &DigitalInputPins[i];

        /* Let the watchdog know we're not crashed */
	WDT.alive();

        if (pPin->id != input)
            continue;

        if (!mcp23017Present[pPin->i2caddr - 0x20])
            break;

        Wire.lock();
        enabled = pPin->enabled;
        Wire.unlock();

        return enabled;
    }

    Debug.printf("invalid input %d!!!!\n", input);
    return false;
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
        }
    }

    if (forcePublish == 0)
        forcePublish = FORCE_PUBLISH_ANALOG_IVL;

    Wire.unlock();
}

bool IOExpansion::updateResource(String resource, String value)
{
    if (resource.startsWith("outputD"))
    {
        resource = resource.substring(6);
        int out = resource.toInt();
        bool result = setDigOutput(out, value.equals("on"));
        Debug.printf("Set digital output: [%d] %s%s\n",
                     out, value.c_str(), result ? "" : " FAILED");
        return result;
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
        bool enabled = getDigInput(input);
        Debug.printf("Get input %d: %sABLED\n",
                     input, enabled ? "EN" : "DIS");
        return enabled ? "on" : "off";
    }
    /* Analog input */
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
        bool enabled = getDigOutput(output);
        Debug.printf("Get output %d: %sABLED\n",
                     output, enabled ? "EN" : "DIS");
        return enabled ? "on" : "off";
    }

    Debug.println("ERROR: GetValue for an unknown object");
    return "invalid";
}

bool IOExpansion::toggleResourceValue(String resource)
{
    if (resource.startsWith("outputD"))
    {
        resource = resource.substring(6);
        int out = resource.toInt();
        bool result = toggleDigOutput(out);
        Debug.printf("Toggle digital output: [%d]%s\n",
                     out, result ? "" : " FAILED");
        return result;
    }
    else
    {
        Debug.println("ERROR: Only digital outputs can be toggled");
        return false;
    }

    return true;
}

IOExpansion Expansion;
