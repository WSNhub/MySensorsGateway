#ifndef INCLUDE_WIFI_H_
#define INCLUDE_WIFI_H_

#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>

typedef Delegate<void(bool)> NetworkStateChangeDelegate;

class NetworkClass
{
  public:
    NetworkClass() : ntpClient(NTP_DEFAULT_SERVER,
                               0,
                               NtpTimeResultDelegate(
                                   &NetworkClass::ntpTimeResultHandler,
                                   this))
    {
        connected = false;
    };

    void begin(NetworkStateChangeDelegate dlg = NULL);
    void softApEnable();
    void softApDisable();
    void reconnect(int delayMs);
    void handleEvent(System_Event_t *e);

    IPAddress getClientIP();
    IPAddress getClientMask();
    IPAddress getClientGW();
    
  private:
    void portalLoginHandler(HttpClient& client, bool successful);
    void connect();
    void ntpTimeResultHandler(NtpClient& client, time_t ntpTime);

  private:
    NetworkStateChangeDelegate changeDlg;
    bool                    connected = false;
    bool                    haveIp = false;
    HttpClient              portalLogin;
    Timer                   reconnectTimer;
    NtpClient               ntpClient;
};

extern NetworkClass Network;

#endif //INCLUDE_WIFI_H_
