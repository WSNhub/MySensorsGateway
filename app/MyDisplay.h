#ifndef INCLUDE_DISPLAY_H_
#define INCLUDE_DISPLAY_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

class MyDisplay
{
  public:
    void  begin();

  private:
    void  update();

  private:
    bool  displayFound = FALSE;

    Timer displayTimer;
};

extern MyDisplay Display;
#endif //INCLUDE_DISPLAY_H_
