#ifndef INCLUDE_RTC_H_
#define INCLUDE_RTC_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

typedef Delegate<void(String, String)> RTCChangeDelegate;

class RTClock
{
  public:
    void begin(RTCChangeDelegate dlg = NULL);
    void setTime(uint32_t ts);
    uint32_t getTime();

  private:
    /* RTC */
    void checkState();

  private:
    RTCChangeDelegate changeDlg;
    bool              RTCFound = FALSE;

    Timer             checkTimer;
};

extern RTClock Clock;

#endif //INCLUDE_RTC_H_
