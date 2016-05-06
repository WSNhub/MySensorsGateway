/**
   MyStatus.h
 */


#ifndef MyStatus_h
#define MyStatus_h

#include "MySensors/MyConfig.h"
#include "MySensors/MySensor.h"

typedef enum
{
    systemReady = 1,
    downloadStarted,
    downloadFailed,
    downloadSuccess
} eFirmwareState;

class MyStatus
{
  public:
    MyStatus();
    void begin();

    void registerHttpHandlers(HttpServer &server);
    void onWsGetStatus (WebSocket& socket, const String& message);
    void onWsGetDldStatus (WebSocket& socket, const String& message);
    void setStartupTime (const String& timeStr);
    void updateGWIpConnection (const String& ipAddrStr, const String& status);
    void updateMqttConnection (const String& ipAddrStr, const String& status);
    void updateDetectedSensors (int nodeUpdate, int sensorUpdate);
    void updateFreeHeapSize (uint32 freeHeap);
    void updateRfPackets (int rx, int tx);
    void updateMqttPackets (int rx, int tx);
    void setFirmwareDldStart (int trial);
    void setFirmwareDldEnd (bool isSuccess, int trial);

    void notifyCounters();
    
  protected:
    String makeJsonKV(const String& key, const String& value);
    String makeJsonStart();
    String makeJsonEnd();
    void notifyUpdate(const String& statusStr);
    void notifyKeyValue(const String& key, const String& value);
    String getFirmwareStateString (eFirmwareState state);

  private:
    int started;
    eFirmwareState firmwareState;
    int  firmwareTrial;
    String systemStartTime;
    
    int numDetectedNodes;
    int numDetectedSensors;
    //uint32 numRfPktRx;
    //uint32 numRfPktTx;
    //uint32 numMqttPktRx;
    //uint32 numMqttPktTx;
    
    uint32 freeHeapSize;
    
	  Timer updateTimer;
};

MyStatus& getStatusObj();


#endif
