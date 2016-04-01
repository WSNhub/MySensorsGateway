/**
   MyStatus.h
 */


#ifndef MyStatus_h
#define MyStatus_h

#include "MySensors/MyConfig.h"
#include "MySensors/MySensor.h"


class MyStatus
{
  public:
    MyStatus();

    void onWsGetStatus (WebSocket& socket, const String& message);
    void setStartupTime (const String& timeStr);
    void updateGWIpConnection (const String& ipAddrStr, const String& status);
    void updateMqttConnection (const String& ipAddrStr, const String& status);
    void updateDetectedSensors (int nodeUpdate, int sensorUpdate);
    
  protected:
    String makeJsonKV(const String& key, const String& value);
    String makeJsonStart();
    String makeJsonEnd();

  private:
    String systemStartTime;
    
    int numDetectedNodes;
    int numDetectedSensors;
};


#endif
