#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <AppSettings.h>
#include <mqtt.h>
#include <MyGateway.h>
#include <HTTP.h>

// Forward declarations
void onMessageReceived(String topic, String message);

// MQTT client
MqttClient *mqtt = NULL;
char clientId[33];
bool MqttConfigured = FALSE;
bool MqttIsConnected = FALSE;

unsigned long mqttPktRx = 0;
unsigned long mqttPktTx = 0;

void ICACHE_FLASH_ATTR mqttPublishMessage(String topic, String message)
{
    if (!mqtt)
        return;

    mqtt->publish(AppSettings.mqttSensorPfx + String("/") + topic, message);
    mqttPktTx++;
}

void ICACHE_FLASH_ATTR mqttPublishVersion()
{
    mqtt->publish(String("/") + clientId + String("/version"),
                  "MySensors gateway");
    mqttPktTx++;
}

// Callback for messages, arrived from MQTT server

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
    if (topic.startsWith(AppSettings.mqttControllerPfx + "/"))
    {
        mqttPktRx++;

        String node   = getValue(topic, '/', 1);
        String sensor = getValue(topic, '/', 2);
        String type   = getValue(topic, '/', 3);
        Debug.println();
        Debug.println();
        Debug.println();
        Debug.println(node);
        Debug.println(sensor);
        Debug.println(type);
        Debug.println(message);
        //strip leading V_
        type.remove(0,2);
        Debug.println(MyGateway::getSensorTypeFromString(type));

        updateSensorStateInt(node.toInt(), sensor.toInt(),
                             MyGateway::getSensorTypeFromString(type),
                             message.toInt());
    }

    if (topic.equals(String("/?")))
    {
        mqttPublishVersion();
        return;
    }

    Debug.println("RX: " + topic + " = " + message);
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
        MqttIsConnected = mqtt->connect(clientId, AppSettings.mqttUser, AppSettings.mqttPass);
        mqtt->subscribe("#");
        mqttPublishVersion();
        MqttConfigured = TRUE;
    }
}

void ICACHE_FLASH_ATTR checkMqttClient()
{
    if (mqtt && mqtt->isProcessing())
        return;

    startMqttClient();
}

void onMqttConfig(HttpRequest &request, HttpResponse &response)
{
    AppSettings.load();
    MqttConfigured = FALSE;

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    if (request.getRequestMethod() == RequestMethod::POST)
    {
        AppSettings.mqttUser = request.getPostParameter("user");
        AppSettings.mqttPass = request.getPostParameter("password");
        AppSettings.mqttServer = request.getPostParameter("server");
        AppSettings.mqttPort = atoi(request.getPostParameter("port").c_str());
        AppSettings.mqttSensorPfx = request.getPostParameter("sensorPfx");
        AppSettings.mqttControllerPfx = request.getPostParameter("controllerPfx");

        AppSettings.save();
        if (AppSettings.wired || WifiStation.isConnected())
            startMqttClient();
    }

    TemplateFileStream *tmpl = new TemplateFileStream("mqtt.html");
    auto &vars = tmpl->variables();

    vars["user"] = AppSettings.mqttUser;
    vars["password"] = AppSettings.mqttPass;
    vars["server"] = AppSettings.mqttServer;
    vars["port"] = AppSettings.mqttPort;
    vars["sensorPfx"] = AppSettings.mqttSensorPfx;
    vars["controllerPfx"] = AppSettings.mqttControllerPfx;
    response.sendTemplate(tmpl); // will be automatically deleted
}

void ICACHE_FLASH_ATTR mqttRegisterHttpHandlers(HttpServer &server)
{
    server.addPath("/mqttconfig", onMqttConfig);
}
bool isMqttConnected(void) 
{
  if (mqtt == NULL)
    return (FALSE);
  else
    return((mqtt->getConnectionState() == eTCS_Connected));
}

bool isMqttConfigured(void) {return(MqttConfigured);}
String MqttServer(void) {return(AppSettings.mqttServer);}
