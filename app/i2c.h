#ifndef INCLUDE_ICC_H_
#define INCLUDE_ICC_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

#define FORCE_PUBLISH_DIG_IVL 600
#define FORCE_PUBLISH_ANALOG_IVL 60

typedef Delegate<void(String, String)> I2CChangeDelegate;
extern Mutex i2cmutex;

class MyI2C
{
  public:
    void begin(I2CChangeDelegate dlg = NULL);

  private:
    /* OLED */
    void showOLED();

  private:
    I2CChangeDelegate changeDlg;

    bool              lcdFound = FALSE;
    bool              OLEDFound = FALSE;

    Timer             i2cOLEDTimer;
};

#endif //INCLUDE_ICC_H_
