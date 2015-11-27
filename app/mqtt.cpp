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
void ICACHE_FLASH_ATTR onMessageReceived(String topic, String message)
{
    /*
     * Supported topics:
     *   /? => send version info
     *   <MQTTPREFIX>/<NODEID>/<SENSOR_ID>/<SENSOR_TYPE>/<VALUE>
     */

    //MyMQTT/22/1/V_LIGHT
    if (topic.equals("MyMQTT/22/1/V_LIGHT"))
    {
        Serial.print("Sending state \"");
        Serial.print(message);
        Serial.println("\" to led");
        SendToSensor(message);
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


