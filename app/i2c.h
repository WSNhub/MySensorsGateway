#ifndef INCLUDE_ICC_H_
#define INCLUDE_ICC_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

#define FORCE_PUBLISH_DIG_IVL 600
#define FORCE_PUBLISH_ANALOG_IVL 60

extern Mutex i2cmutex;

class MyI2C
{
  public:
    void begin();
};

#endif //INCLUDE_ICC_H_
