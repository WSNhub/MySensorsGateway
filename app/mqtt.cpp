#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

// Forward declarations
void startMqttClient();
void onMessageReceived(String topic, String message);

// MQTT client
MqttClient *mqtt = NULL;
char clientId[33];

#define MQTTPREFIX "MyMQTT"

//<MQTTPREFIX>/<NODEID>/<SENSOR_ID>/<SENSOR_TYPE>/<VALUE>
void ICACHE_FLASH_ATTR mqttPublishMessage(String topic, String message)
{
    if (!mqtt)
        return;

    mqtt->publish(MQTTPREFIX + String("/") + topic, message);
}

void ICACHE_FLASH_ATTR mqttPublishVersion()
{
    mqtt->publish(String("/") + clientId + String("/version"),
                  "MySensors gateway");
}

// Callback for messages, arrived from MQTT server
void SendToSensor(String value);

String getValue(String data, char separator, int index)
{
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

int updateSensorStateInt(int node, int sensor, int type, int value);
int getTypeFromString(String type);

void ICACHE_FLASH_ATTR onMessageReceived(String topic, String message)
{
    /*
     * Supported topics:
     *   /? => send version info
     *   <MQTTPREFIX>/<NODEID>/<SENSOR_ID>/<SENSOR_TYPE>/<VALUE>
     */

    //MyMQTT/22/1/V_LIGHT
    if (topic.startsWith("MyMQTT/"))
    {
        String node   = getValue(topic, '/', 1);
        String sensor = getValue(topic, '/', 2);
        String type   = getValue(topic, '/', 3);
        Serial.println();
        Serial.println();
        Serial.println();
        Serial.println(node);
        Serial.println(sensor);
        Serial.println(type);
        Serial.println(message);

        updateSensorStateInt(node.toInt(), sensor.toInt(),
                             getTypeFromString(type),
                             message.toInt());
    }

    if (topic.equals(String("/?")))
    {
        mqttPublishVersion();
        return;
    }

    Serial.println(topic);
}

// Run MQTT client
void ICACHE_FLASH_ATTR startMqttClient()
{
    if (mqtt)
        delete mqtt;

    AppSettings.load();
    if (!AppSettings.mqttServer.equals(String("")) && AppSettings.mqttPort != 0)
    {
        sprintf(clientId, "ESP_%08X", system_get_chip_id());
        mqtt = new MqttClient(AppSettings.mqttServer, AppSettings.mqttPort, onMessageReceived);
        mqtt->connect(clientId, AppSettings.mqttUser, AppSettings.mqttPass);
        mqtt->subscribe("#");
        //mqtt->subscribe("/?");
        mqtt->subscribe(String("/") + clientId + String("/#"));
        mqttPublishVersion();
    }
}

void ICACHE_FLASH_ATTR checkMqttClient()
{
    if (mqtt && mqtt->isProcessing())
        return;

    startMqttClient();
}


