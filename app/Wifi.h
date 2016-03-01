#ifndef INCLUDE_WIFI_H_
#define INCLUDE_WIFI_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

typedef Delegate<void(bool)> WifiStateChangeDelegate;

class WifiClass
{
  public:
    WifiClass() { connected = false; };

    void begin(WifiStateChangeDelegate dlg = NULL);
    void softApEnable();
    void reconnect(int delayMs);
    void handleEvent(System_Event_t *e);

  private:
    void portalLoginHandler(HttpClient& client, bool successful);
    void connect();

  private:
    WifiStateChangeDelegate changeDlg;
    bool                    connected = false;
    bool                    haveIp = false;
    HttpClient              portalLogin;
    Timer                   reconnectTimer;
};

extern WifiClass Wifi;

#endif //INCLUDE_WIFI_H_
