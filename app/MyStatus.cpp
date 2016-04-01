#include <AppSettings.h>
#include <user_config.h>
#include <SmingCore.h>
#include "MyStatus.h"
#include "Network.h"
#include "HTTP.h"

extern MyStatus myStatus;

MyStatus::MyStatus()
{
    started = 0;
    freeHeapSize = 0;
    numDetectedNodes = 0;
    numDetectedSensors = 0;
}

void MyStatus::begin()
{
  started = 1;
}

String MyStatus::makeJsonKV(const String& key, const String& value)
{
    String kvStr = String("{\"key\": \"" + key + "\",");
    kvStr += String("\"value\": \"" + value + "\"}");
    return kvStr;
}
String MyStatus::makeJsonStart()
{
    String str("{\"type\": \"status\", \"data\" : [");
    return str;
}

String MyStatus::makeJsonEnd()
{
    String str("]}");
    return str;
}

void MyStatus::notifyUpdate(const String& statusStr)
{
  if (started)
  {
    String str = makeJsonStart();
    str += statusStr;
    str += makeJsonEnd();
    HTTP.notifyWsClients(str);
  }
}

void MyStatus::notifyKeyValue(const String& key, const String& value)
{
  if (started)
  {
    String str = makeJsonStart();
    str += makeJsonKV (key, value);
    str += makeJsonEnd();
    HTTP.notifyWsClients(str);
  }
}

void MyStatus::onWsGetStatus (WebSocket& socket, const String& message)
{
    bool dhcp = AppSettings.dhcp;

    notifyKeyValue ("ssid", AppSettings.ssid);
    notifyKeyValue ("wifiStatus", isNetworkConnected ? "Connected" : "Not connected");
    
    if (!Network.getClientIP().isNull())
    {
        notifyKeyValue ("gwIp", Network.getClientIP().toString());
        if (dhcp)
        {
          notifyKeyValue ("gwIpStatus", "From DHCP");
        }
        else
        {
          notifyKeyValue ("gwIpStatus", "Static");
        }
    }
    else
    {
        notifyKeyValue ("gwIp", "0.0.0.0");
        notifyKeyValue ("gwIpStatus", "not configured");
    }


    String statusStr = makeJsonStart();
    statusStr += makeJsonKV ("detNodes", String(numDetectedNodes));
    statusStr += String(",");
    statusStr += makeJsonKV ("detSensors", String(numDetectedSensors));

    statusStr += String(",");
    statusStr += makeJsonKV ("systemFreeHeap", String(system_get_free_heap_size()));
    statusStr += String(",");
    statusStr += makeJsonKV ("systemStartTime", systemStartTime);
    statusStr += makeJsonEnd();
    socket.sendString(statusStr);
}

void MyStatus::setStartupTime (const String& timeStr)
{
    if (systemStartTime.equals(""))
    {
      systemStartTime = timeStr;
      notifyKeyValue ("systemStartTime", timeStr);
    }
}

void MyStatus::updateGWIpConnection (const String& ipAddrStr, const String& status)
{
}

void MyStatus::updateMqttConnection (const String& ipAddrStr, const String& status)
{
}

void MyStatus::updateDetectedSensors (int nodeUpdate, int sensorUpdate)
{
    numDetectedNodes += nodeUpdate;
    numDetectedSensors += sensorUpdate;
    
    String statusStr = makeJsonKV ("detNodes", String(numDetectedNodes));
    statusStr += String(",");
    statusStr += makeJsonKV ("detSensors", String(numDetectedSensors));
    notifyUpdate (statusStr);
}

void MyStatus::updateFreeHeapSize (uint32 freeHeap)
{
    if (freeHeapSize != freeHeap)
    {
      freeHeapSize = freeHeap;
      notifyKeyValue ("systemFreeHeap", String(freeHeapSize));
    }
}

MyStatus& getStatusObj()
{
    return (myStatus);
}
