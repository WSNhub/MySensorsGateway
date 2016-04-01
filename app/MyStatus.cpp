#include <AppSettings.h>
#include <user_config.h>
#include <SmingCore.h>
#include "MyStatus.h"
#include "HTTP.h"


MyStatus::MyStatus()
{
    numDetectedNodes = 0;
    numDetectedSensors = 0;
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

void MyStatus::onWsGetStatus (WebSocket& socket, const String& message)
{
    String statusStr = makeJsonStart();

    statusStr += makeJsonKV ("detNodes", String(numDetectedNodes));
    statusStr += String(",");
    statusStr += makeJsonKV ("detSensors", String(numDetectedSensors));

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
    
    String statusStr = makeJsonStart();
    statusStr += makeJsonKV ("detNodes", String(numDetectedNodes));
    statusStr += String(",");
    statusStr += makeJsonKV ("detSensors", String(numDetectedSensors));
    statusStr += makeJsonEnd();
    HTTP.notifyWsClients(statusStr);
}

