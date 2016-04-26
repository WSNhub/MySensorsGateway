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
    firmwareTrial=0;
    freeHeapSize = 0;
    numDetectedNodes = 0;
    numDetectedSensors = 0;
    //numRfPktRx = 0;
    //numRfPktTx = 0;
    //numMqttPktRx = 0;
    //numMqttPktTx = 0;
}

void MyStatus::begin()
{
  started = 1;
	updateTimer.initializeMs(2000,
			TimerDelegate(&MyStatus::notifyCounters, this));
}

void MyStatus::registerHttpHandlers(HttpServer &server)
{
    HTTP.addWsCommand("getDldStatus", WebSocketMessageDelegate(&MyStatus::onWsGetDldStatus, this));
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
  else
  {
    String info = String ("No update because started=") + String (started)
        + String (" isFirmwareDld=") + String (isFirmwareDld);
    Debug.println(info);
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
  else
  {
    String info = String ("No update because started=") + String (started)
        + String (" isFirmwareDld=") + String (isFirmwareDld);
    Debug.println(info);
  }
}

void MyStatus::onWsGetDldStatus (WebSocket& socket, const String& message)
{
    String str("{\"type\": \"firmware\", \"data\" : [");
    if (isFirmwareDld)
    {
      String val = String ("Downloading firmware (trial=") + String (firmwareTrial) + String (")");
      str += makeJsonKV ("firmwareSt", val);
    }
    else
    {
      str += makeJsonKV ("firmwareSt", "...");
    }
    str += String(",");
    str += makeJsonKV ("systemVersion",build_git_sha);
    str += String(",");
    str += makeJsonKV ("systemBuild",build_time);
    str += makeJsonEnd();
    socket.sendString(str);
}

void MyStatus::onWsGetStatus (WebSocket& socket, const String& message)
{
    bool dhcp = AppSettings.dhcp;
    char buf [200];
    String statusStr;
    
    statusStr = makeJsonStart();
    statusStr += makeJsonKV ("ssid", AppSettings.ssid);
    statusStr += String(",");
    statusStr += makeJsonKV ("wifiStatus", isNetworkConnected ? "Connected" : "Not connected");
    statusStr += String(",");
    
    if (!Network.getClientIP().isNull())
    {
        statusStr += makeJsonKV ("gwIp", Network.getClientIP().toString());
        statusStr += String(",");
        if (dhcp)
        {
          statusStr += makeJsonKV ("gwIpStatus", "From DHCP");
        }
        else
        {
          statusStr += makeJsonKV ("gwIpStatus", "Static");
        }
    }
    else
    {
        statusStr += makeJsonKV ("gwIp", "0.0.0.0");
        statusStr += String(",");
        statusStr += makeJsonKV ("gwIpStatus", "not configured");
    }
    statusStr += makeJsonEnd();
    socket.sendString(statusStr);

    // ---------------
    statusStr = makeJsonStart();
    if (AppSettings.mqttServer != "")
    {
        statusStr += makeJsonKV ("mqttIp", AppSettings.mqttServer);
        statusStr += String(",");
        statusStr += makeJsonKV ("mqttStatus", isMqttConnected() ? "Connected":"Not connected");
    }
    else
    {
        statusStr += makeJsonKV ("mqttIp", "0.0.0.0");
        statusStr += String(",");
        statusStr += makeJsonKV ("mqttStatus", "Not configured");
    }


    uint64_t rfBaseAddress = GW.getBaseAddress();
    uint32_t rfBaseLow = (rfBaseAddress & 0xffffffff);
    uint8_t  rfBaseHigh = ((rfBaseAddress >> 32) & 0xff);
    if (AppSettings.useOwnBaseAddress)
      sprintf (buf, "%02x%08x (private)", rfBaseHigh, rfBaseLow);
    else
      sprintf (buf, "%02x%08x (default)", rfBaseHigh, rfBaseLow);

    statusStr += String(",");
    statusStr += makeJsonKV ("baseAddress", buf);
    statusStr += String(",");
    statusStr += makeJsonKV ("radioStatus", "?");
    statusStr += makeJsonEnd();
    socket.sendString(statusStr);
    
    // ---------------
    statusStr = makeJsonStart();
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
    int slot = rboot_get_current_rom();
    statusStr = makeJsonStart();
    statusStr += makeJsonKV ("systemVersion", build_git_sha);
    statusStr += String(",");
    statusStr += makeJsonKV ("systemBuild", build_time);
    statusStr += String(",");
    statusStr += makeJsonKV ("currentRomSlot", String(slot));
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
    String statusStr = makeJsonKV ("rfRx", String(rfPacketsRx)); // numRfPktRx
    statusStr += String(",");
    statusStr += makeJsonKV ("rfTx", String(rfPacketsTx)); //numRfPktTx
    notifyUpdate (statusStr);
    notifyKeyValue ("mqttRx", String(mqttPktRx)); // numMqttPktRx
    notifyKeyValue ("mqttTx", String(mqttPktTx)); // numMqttPktTx
}

void MyStatus::updateRfPackets (int rx, int tx)
{
    //numRfPktRx += rx;
    //numRfPktTx += tx;
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
    firmwareTrial = trial;
    String str("{\"type\": \"firmware\", \"data\" : [");
    str += String ("{\"key\": \"firmwareSt\",");
    str += String("\"value\": \"Firmware download started, trial=") + String(trial)+ String("\"}");
    str += String("]}");
    Debug.println(str.c_str());
    HTTP.notifyWsClients(str);
}

void MyStatus::setFirmwareDldEnd (bool isSuccess, int trial)
{
    isFirmwareDld = false;
    String str("{\"type\": \"firmware\", \"data\" : [");
    str += String ("{\"key\": \"firmwareSt\",");
    if (isSuccess)
      str += String("\"value\": \"Firmware download finished\"}");
    else
      str += String("\"value\": \"Firmware download failed (trial=")
             + String(trial) + String(")\"}");
    
    str += String("]}");
    Debug.println(str.c_str());
    HTTP.notifyWsClients(str);
}


MyStatus& getStatusObj()
{
    return (myStatus);
}
