#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

// Forward declarations
void startMqttClient();
void onMessageReceived(String topic, String message);
void OtaUpdate();

// MQTT client
MqttClient *mqtt = NULL;
char clientId[33];

void ICACHE_FLASH_ATTR mqttPublishMessage(String topic, String message)
{
    if (!mqtt)
        return;

    mqtt->publish(String("/") + clientId + String("/") + topic,
                  message);
}

void ICACHE_FLASH_ATTR mqttPublishVersion()
{
    mqtt->publish(String("/") + clientId + String("/version"),
                  "cloud-o-mation C-001");
}

bool ICACHE_FLASH_ATTR i2cSetMcpOutput(uint8_t output, bool enable);
bool ICACHE_FLASH_ATTR i2cSetMcpOutputInvert(uint8_t output, bool invert);
bool ICACHE_FLASH_ATTR i2cSetMcpInputInvert(uint8_t input, bool invert);

// Callback for messages, arrived from MQTT server
void ICACHE_FLASH_ATTR onMessageReceived(String topic, String message)
{
    /*
     * Supported topics:
     *   /?                => send version info
     */

    if (topic.equals(String("/?")))
    {
        mqttPublishVersion();
        return;
    }

    if (topic.startsWith("version"))
    {
        /* ignore */
        return;
    }

    Serial.print("Unrecognized topic: ");
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
        mqtt->subscribe("/?");
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


