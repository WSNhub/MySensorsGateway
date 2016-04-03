#include <AppSettings.h>
#include <user_config.h>
#include <SmingCore.h>
#include "MyGateway.h"
#include "MyStatus.h"
#include "Network.h"
#include "HTTP.h"
#include <mqtt.h>

extern MyStatus myStatus;

MyStatus::MyStatus()
{
    started = 0;
    isFirmwareDld = false;
    freeHeapSize = 0;
    numDetectedNodes = 0;
    numDetectedSensors = 0;
    numRfPktRx = 0;
    numRfPktTx = 0;
    //numMqttPktRx = 0;
    //numMqttPktTx = 0;
}

void MyStatus::begin()
{
  started = 1;
	updateTimer.initializeMs(2000,
			TimerDelegate(&MyStatus::notifyCounters, this));
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
  if (started && !isFirmwareDld)
  {
    String str = makeJsonStart();
    str += statusStr;
    str += makeJsonEnd();
    HTTP.notifyWsClients(str);
  }
}

void MyStatus::notifyKeyValue(const String& key, const String& value)
{
  if (started && !isFirmwareDld)
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
    char buf [200];

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

    if (AppSettings.mqttServer != "")
    {
        notifyKeyValue ("mqttIp", AppSettings.mqttServer);
        notifyKeyValue ("mqttStatus", isMqttConnected() ? "Connected":"Not connected");
    }
    else
    {
        notifyKeyValue ("mqttIp", "0.0.0.0");
        notifyKeyValue ("mqttStatus", "Not configured");
    }


    uint64_t rfBaseAddress = GW.getBaseAddress();
    uint32_t rfBaseLow = (rfBaseAddress & 0xffffffff);
    uint8_t  rfBaseHigh = ((rfBaseAddress >> 32) & 0xff);
    if (AppSettings.useOwnBaseAddress)
      sprintf (buf, "%02x%08x (private)", rfBaseHigh, rfBaseLow);
    else
      sprintf (buf, "%02x%08x (default)", rfBaseHigh, rfBaseLow);
    notifyKeyValue ("baseAddress", buf);
    notifyKeyValue ("radioStatus", "?");

    // ---------------
    String statusStr = makeJsonStart();
    statusStr += makeJsonKV ("detNodes", String(numDetectedNodes));
    statusStr += String(",");
    statusStr += makeJsonKV ("detSensors", String(numDetectedSensors));
    statusStr += String(",");
    statusStr += makeJsonKV ("rfRx", String(rfPacketsRx)); 
    statusStr += String(",");
    statusStr += makeJsonKV ("rfTx", String(rfPacketsTx));
    statusStr += String(",");
    statusStr += makeJsonKV ("mqttRx", String(mqttPktRx));
    statusStr += String(",");
    statusStr += makeJsonKV ("mqttTx", String(mqttPktTx));
    statusStr += makeJsonEnd();
    socket.sendString(statusStr);

    // ---------------
    sprintf (buf, "%x", system_get_chip_id());
    statusStr = makeJsonStart();
    statusStr += makeJsonKV ("systemVersion", build_git_sha);
    statusStr += String(",");
    statusStr += makeJsonKV ("systemBuild", build_time);
    statusStr += String(",");
    statusStr += makeJsonKV ("systemChipId", buf);
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
    notifyKeyValue ("gwIp", ipAddrStr);
    notifyKeyValue ("gwIpStatus", status);
}

void MyStatus::updateMqttConnection (const String& ipAddrStr, const String& status)
{
    notifyKeyValue ("mqttIp", ipAddrStr);
    notifyKeyValue ("mqttStatus", status);
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

void MyStatus::notifyCounters()
{
    String statusStr = makeJsonKV ("rfRx", String(numRfPktRx)); 
    statusStr += String(",");
    statusStr += makeJsonKV ("rfTx", String(rfPacketsTx)); //numRfPktTx
    notifyUpdate (statusStr);
    notifyKeyValue ("mqttRx", String(mqttPktRx)); // numMqttPktRx
    notifyKeyValue ("mqttTx", String(mqttPktTx)); // numMqttPktTx
}

void MyStatus::updateRfPackets (int rx, int tx)
{
    numRfPktRx += rx;
    numRfPktTx += tx;
    if (! updateTimer.isStarted())
    {
      updateTimer.startOnce();
    }
}

void MyStatus::updateMqttPackets (int rx, int tx)
{
    //numMqttPktRx += rx;
    //numMqttPktTx += tx;
    if (! updateTimer.isStarted())
    {
      updateTimer.startOnce();
    }
}


void MyStatus::updateFreeHeapSize (uint32 freeHeap)
{
    if (freeHeapSize != freeHeap)
    {
      freeHeapSize = freeHeap;
      notifyKeyValue ("systemFreeHeap", String(freeHeapSize));
    }
}

void MyStatus::setFirmwareDldStart (int trial)
{
    isFirmwareDld = true;
    String str("{\"type\": \"firmware\", \"data\" : [");
    str += String ("{\"key\": \"firmwareSt\",");
    str += String("\"value\": \"Firmware download started, trial=") + String(trial)+ String("\"}");
    str += String("]}");
    HTTP.notifyWsClients(str);
}

void MyStatus::setFirmwareDldEnd (bool isSuccess)
{
    isFirmwareDld = false;
    String str("{\"type\": \"firmware\", \"data\" : [");
    str += String ("{\"key\": \"firmwareSt\",");
    if (isSuccess)
      str += String("\"value\": \"Firmware download finished\"}");
    else
      str += String("\"value\": \"Firmware download failed !\"}");
    
    str += String("]}");
    HTTP.notifyWsClients(str);
}


MyStatus& getStatusObj()
{
    return (myStatus);
}
