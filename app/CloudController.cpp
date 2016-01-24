#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <CloudController.h>
#include <AppSettings.h>
#include <globals.h>

void CloudController::begin()
{
    checkTimer.initializeMs(
        1000,
        TimerDelegate(&CloudController::checkMqttClient,
	              this)).start(true);
}

void CloudController::notifyChange(String object, String value)
{
    if (!mqtt)
        return;

    mqtt->publish(mqtt_thing_topic,
                  "{\"values\":[{\"key\":\"" + object + "\",\"value\":\""+value+"\"}]}");
}

void CloudController::registerHttpHandlers(HttpServer &server)
{
    //mqttRegisterHttpHandlers(server);
}

void CloudController::registerCommandHandlers()
{
    //
}

void CloudController::mqttPublishIdAndVersion()
{
    char id[16];
    sprintf(id, "%x", system_get_chip_id());

    incarnation++;
    mqtt->publish(mqtt_thing_topic,
                  "{\"values\":[{\"key\":\"id\",\"value\":\""+String(id)+"\"}]}");
    mqtt->publish(mqtt_thing_topic,
                  "{\"values\":[{\"key\":\"version\",\"value\":\"C-001 "+String(build_git_sha)+" "+String(build_time)+" ("+ incarnation +")\"}]}");
}

void CloudController::startMqttClient()
{
    String token = ""; //TODO AppSettings.getDeviceToken();

    if (token.equals(""))
        return;

    /* v2/things/{THING_TOKEN} */
    if (!mqtt_thing_topic)
    {
        mqtt_thing_topic = (char *)malloc (10 + token.length() + 1);
        if (!mqtt_thing_topic)
            return;
        sprintf(mqtt_thing_topic, "v2/things/%s", token.c_str());
        Debug.printf("TOPIC: %s\n", mqtt_thing_topic);
    }

    state = CloudControllerStateMqttConnecting;

    if (!mqtt)
        mqtt = new MqttClient("mqtt.thethings.io", 1883,
                              MqttStringSubscriptionCallback(&CloudController::onMessageReceived, this));

    Debug.printf("===> Starting MQTT client\n");
    mqtt->connect(token.c_str());
}

void CloudController::checkMqttClient()
{
    if (state < CloudControllerStateActivated)
        return;

    if (!mqtt || !mqtt->isProcessing() || numChecks > 10)
    {
        startMqttClient();
        return;
    }

    if (state == CloudControllerStateMqttConnecting)
    {
        bool connected = (mqtt->getConnectionState() == eTCS_Connected);
        if (connected)
        {
            state = CloudControllerStateMqttConnected;
            numChecks = 0;
        }
        else
        {
            numChecks ++;
        }
    }

    if (state == CloudControllerStateMqttConnected)
    {
        Debug.printf("===> Client connected, subscribing... (2)\n");
        bool subscribed = mqtt->subscribe(mqtt_thing_topic);
        if (subscribed)
        {
            state = CloudControllerStateMqttSubscribed;
            mqttPublishIdAndVersion();
        }
    }
}

void CloudController::stopMqttClient()
{
    Debug.printf("===> Stopping MQTT client\n");

    if (mqtt)
        delete(mqtt);
    mqtt = NULL;

    if (mqtt_thing_topic)
        delete(mqtt_thing_topic);
    mqtt_thing_topic = NULL;

    state = CloudControllerStateActivated;
}

void CloudController::onMessageReceived(String topic, String message)
{
    //if (!topic.equals(mqtt_thing_topic))
    {
        Debug.printf("Unrecognized topic: %s message: %s",
               topic.c_str(), message.c_str());
        return;
    }

#if 0
    //[{"value":{"output":"d1"},"key":"set"}]
    Debug.println(message);
    DynamicJsonBuffer jsonBuffer;
    JsonArray& root = jsonBuffer.parseArray((char *)message.c_str());
    if (!root.success())
    {
        Debug.printf("Parsing failed\n");
        return;
    }

    int i = 0;
    while (true)
    {
        JsonObject& entry = root[i];
        if (!entry.success())
            break;

        String key = (const char *)entry["key"];
        if (key.equals("set"))
        {
            JsonObject& value = entry["value"];
            
            String output = (const char *)value["output"];
            String val = (const char *)value["value"];

            if (output.startsWith("d"))
            {
                output = output.substring(1);
                int out = output.toInt();
                if (out < 1 || out > 56)
                {
                    Debug.printf("invalid output: d%d", out);
                    goto next_entry;
                }

                Debug.printf("Set digital output: [%d] %s\n", out, val.c_str());
                i2cSetMcpOutput(out, val.equals("on"));
            }
            else if (output.startsWith("a"))
            {
                output = output.substring(1);
                int out = output.toInt();
                if (out < 1 || out > 8)
                {
                    Debug.printf("invalid output: a%d", out);
                    goto next_entry;
                }

                Debug.printf("Set analog output: [%d] %s\n", out, val.c_str());
                //i2cSetPcfOutput(out, val.equals("on"));
            }
            goto next_entry;
        }
        
        if (key.equals("invert"))
        {
            JsonObject& value = entry["value"];
            
            String input = (const char *)value["input"];
            String output = (const char *)value["output"];
            String val = (const char *)value["value"];
            
            if (!output.equals(""))
            {
                if (output.startsWith("d"))
                {
                    output = output.substring(1);
                    int out = output.toInt();
                    if (out < 1 || out > 56)
                    {
                        Debug.printf("invalid output: d%d", out);
                        goto next_entry;
                    }

                    Debug.printf("Invert digital output: [%d] %s\n", out, val.c_str());
                    i2cSetMcpOutputInvert(out, val.equals("yes"));
                }
            }
            else if (!input.equals(""))
            {
                if (input.startsWith("d"))
                {
                    input = input.substring(1);
                    int in = input.toInt();
                    if (in < 1 || in > 56)
                    {
                        Debug.printf("invalid input: d%d", in);
                        goto next_entry;
                    }

                    Debug.printf("Invert digital input: [%d] %s\n", in, val.c_str());
                    i2cSetMcpInputInvert(in, val.equals("yes"));
                }
            }
            goto next_entry;
        }

        if (key.equals("ping"))
        {
            i2cForcePublishAll();
            goto next_entry;
        }

        if (key.equals("upgrade"))
        {
            StartOtaUpdate();
            goto next_entry;
        }

next_entry:
        i++;
    }
#endif
}

CloudController controller;
