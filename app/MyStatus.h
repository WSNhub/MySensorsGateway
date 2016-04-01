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
    void begin();

    void onWsGetStatus (WebSocket& socket, const String& message);
    void setStartupTime (const String& timeStr);
    void updateGWIpConnection (const String& ipAddrStr, const String& status);
    void updateMqttConnection (const String& ipAddrStr, const String& status);
    void updateDetectedSensors (int nodeUpdate, int sensorUpdate);
    void updateFreeHeapSize (uint32 freeHeap);
    
  protected:
    String makeJsonKV(const String& key, const String& value);
    String makeJsonStart();
    String makeJsonEnd();
    void notifyUpdate(const String& statusStr);
    void notifyKeyValue(const String& key, const String& value);

  private:
    int started;
    String systemStartTime;
    
    int numDetectedNodes;
    int numDetectedSensors;
    uint32 freeHeapSize;
};

MyStatus& getStatusObj();


#endif
