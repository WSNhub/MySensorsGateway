#include <AppSettings.h>
#include <user_config.h>
#include <SmingCore.h>
#include "MyGateway.h"
#include "MySensors/MySigningAtsha204.h"
#include "MySensors/MySigningAtsha204Soft.h"
#include "Rule.h"
#include "HTTP.h"

//#define RADIO_CE_PIN 2
//#define RADIO_SPI_SS_PIN 15

MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);
MyHwESP8266 hw;
#if SIGNING_ENABLE
#if ATSHA204I2C 
MySigningAtsha204 signer(true /* requestSignatures */);
#else
uint8_t HMAC_KEY[32] = SIGNING_HMAC;
MySigningAtsha204Soft signer(true /* requestSignatures */,
                         HMAC_KEY);
#endif
#endif


MyGateway::MyGateway() : gw(transport,
                            hw
#ifdef MY_SIGNING_FEATURE
                         ,  signer
#endif
#ifdef WITH_LEDS_BLINKING
                         ,  DEFAULT_RX_LED_PIN, DEFAULT_TX_LED_PIN,
                            DEFAULT_ERR_LED_PIN, DEFAULT_LED_BLINK_PERIOD
#endif
)
{
    nodeIds[0] = true;
    nodeIds[255] = true;
    for (int i = 1; i < 255; i++)
        nodeIds[i] = false;
    
    for (int i = 0; i < MAX_MY_SENSORS; i++)
        memset(&mySensors[i], 0, sizeof(sensor_t));
}

void MyGateway::process()
{
    gw.process();
}

const char * MyGateway::version()
{
   return(LIBRARY_VERSION);
}

String MyGateway::getSensorJson(int index)
{
    String sensorStr = String("{\"type\": \"sensor\", \"data\" : ") +
                       String("{\"id\": ") + String(index+1) +
                       String(",\"node\": ") +
                       String(mySensors[index].node) +
                       String(",\"sensor\": ") +
                       String(mySensors[index].sensor) +
                       String(",\"type\": ") +
                       String(mySensors[index].type) +
                       String(",\"value\": \"");
    if (!mySensors[index].value.equals(""))
        sensorStr += String(mySensors[index].value);
    sensorStr += String("\"}}");

    return sensorStr;
}

void MyGateway::incomingMessage(const MyMessage &message)
{
    msg = message;

    #if MEASURE_ENABLE
    digitalWrite(SCOPE_PIN, true);
    #endif

    // Pass along the message from sensors to serial line
    Debug.printf("RX %d;%d;%d;%d;%d;%s\n",
                 message.sender, message.sensor,
                 mGetCommand(message), mGetAck(message),
                 message.type, message.getString(convBuf));

    if (!msg.isAck())
    {
        
        // we have to check every message if its a newly assigned id or not.
        // Ack on I_ID_RESPONSE does not work, and checking on C_PRESENTATION
        // isn't reliable.
        if (msg.sender != 0 && msg.sender != 255 && !nodeIds[msg.sender])
        {
            numDetectedNodes++;
            Debug.printf("Discovered new node %d\n", msg.sender);
            nodeIds[(uint8_t)msg.sender] = true;
        }

        if (mGetCommand(msg) == C_INTERNAL)
        {
            if (msg.type == I_CONFIG)
            {
                gw.sendRoute(build(msg, msg.sender, 255,
                                   C_INTERNAL, I_CONFIG, 0).set(GW_UNIT));
                return;
            }
            else if (msg.type == I_ID_REQUEST && msg.sender == 255)
            {
                for (int id = GW_FIRST_SENSORID; id <= GW_LAST_SENSORID; id++)
                {
                    if (nodeIds[id] == false)
                    {
                        Debug.printf("Found id %d for new node\n", id);
                        gw.sendRoute(build(msg, msg.sender, 255,
                                           C_INTERNAL, I_ID_RESPONSE,
                                           0).set((uint8_t)id));
                        nodeIds[id] = true;
                        return;
                    }
                }
                return;
            }
        }
        if (message.sender > 0 && message.sender < 255 &&
            message.sensor < 255 &&
            (mGetCommand(msg) == C_SET || mGetCommand(msg) == C_PRESENTATION))
        {
            bool newSensor = true;

            rfPacketsRx++;
            for (int idx = 0; idx < MAX_MY_SENSORS; idx++)
            {
                if (mySensors[idx].node == message.sender &&
                    mySensors[idx].sensor == message.sensor)
                {
                    mySensors[idx].type = message.type;
                    if (mGetCommand(msg) == C_SET)
                    {
                        String newValue = message.getString(convBuf);
                        if (sensorValueChanged &&
                            !newValue.equals(mySensors[idx].value))
                        {
                            sensorValueChanged(idx, newValue);
                        }
                        mySensors[idx].value = newValue;
                        Debug.printf("Updating sensor %d (%d/%d) type %d value %s\n",
                                     idx, mySensors[idx].node, mySensors[idx].sensor,
                                     mySensors[idx].type, mySensors[idx].value.c_str());
                        Rules.processTrigger("sensor"+String(idx+1));
                    }
                    newSensor = false;
                    HTTP.notifyWsClients(getSensorJson(idx));
                    break;
                }
            }

            if (newSensor)
            {
                for (int idx = 0; idx < MAX_MY_SENSORS; idx++)
                {
                    if (mySensors[idx].node == 0 &&
                        mySensors[idx].sensor == 0)
                    {
                        mySensors[idx].node = message.sender;
                        mySensors[idx].sensor = message.sensor;
                        mySensors[idx].type = message.type;
                        if (mGetCommand(msg) == C_SET)
                        {
                            String newValue = message.getString(convBuf);
                            if (sensorValueChanged &&
                                !newValue.equals(mySensors[idx].value))
                            {
                                sensorValueChanged(idx, newValue);
                            }
                            mySensors[idx].value = newValue;
                            HTTP.notifyWsClients(getSensorJson(idx));
                            Rules.processTrigger("sensor"+String(idx+1));
                        }
                        else
                        {
                            HTTP.notifyWsClients(getSensorJson(idx));
                        }                            
                        numDetectedSensors++;
                        Debug.printf("Adding sensor %d (%d/%d) type %d value %s\n",
                                     idx, mySensors[idx].node, mySensors[idx].sensor,
                                     mySensors[idx].type, mySensors[idx].value.c_str());
                        newSensor = false;

                        DynamicJsonBuffer jsonBuffer;
                        JsonObject& root = jsonBuffer.createObject();
                        JsonArray& sensors = jsonBuffer.createArray();
                        root["sensors"] = sensors;
                        for (int s = 0; s < MAX_MY_SENSORS; s++)
                        {
                            String sensorsStr = String(mySensors[s].node) +
                                                "/" +
                                                String(mySensors[s].sensor);
                            sensors.add(sensorsStr);
                        }
                        String out;
                        root.printTo(out);
                        fileSetContent("sensors.json", out);

                        break;
                    }
                }
            }

            if (newSensor)
            {
                Debug.printf("No entry left for new sensor %d/%d type %d value %s\n",
                             message.sender, message.sensor,
                             message.type, message.getString(convBuf));
                return;
            }
        }
        else
        {
            if (mGetCommand(msg)==C_INTERNAL)
                msg.type=msg.type+(S_FIRSTCUSTOM-10); //Special message
        }

        if (msgRx)
            msgRx(message);

    #if MEASURE_ENABLE
    digitalWrite(SCOPE_PIN, false);
    #endif

    }
}

void MyGateway::begin(msgRxDelegate rxDlg, sensorValueChangedDelegate valueChanged)
{
    msgRx = rxDlg;
    sensorValueChanged = valueChanged;
    numDetectedNodes = 0;
    numDetectedSensors = 0;

    if (fileExist("sensors.json"))
    {
        DynamicJsonBuffer jsonBuffer;
        int size = fileGetSize("sensors.json");
        char* jsonString = new char[size + 1];
        fileGetContent("sensors.json", jsonString, size + 1);
        JsonObject& root = jsonBuffer.parseObject(jsonString);
        JsonArray& sensors = root["sensors"];

        for (int i = 0; i < MAX_MY_SENSORS; i++)
        {
            const char *str = sensors[i];
            String sensorStr = str;
            int index = sensorStr.indexOf('/');
            if (index != -1)
            {
                int node = sensorStr.substring(0, index).toInt();
                int sensor = sensorStr.substring(index + 1).toInt();
                mySensors[i].node = node;
                mySensors[i].sensor = sensor;

                if (!nodeIds[node])
                {
                    numDetectedNodes++;
                    nodeIds[node] = true;
                }
            }
        }

        delete[] jsonString;
    }

#if SIGNING_ENABLE
#if ATSHA204I2C 
    signer.personalize();
#endif
#endif

    if (AppSettings.useOwnBaseAddress)
    {
        rfBaseAddress = ((uint64_t)system_get_chip_id()) << 8;
    }
    else
    {
        rfBaseAddress = RF24_BASE_RADIO_ID;
    }

    hw_init();
    gw.begin(msgRxDelegate(&MyGateway::incomingMessage, this),
             0, true, 0, rfBaseAddress);
    processTimer.initializeUs(100, TimerDelegate(&MyGateway::process, this)).start();
}

MyMessage& MyGateway::build (MyMessage &msg, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.destination = destination;
	msg.sender = GATEWAY_ADDRESS;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

boolean MyGateway::sendRoute(MyMessage &msg)
{
    return gw.sendRoute(msg);
}

void MyGateway::onGetSensors(HttpRequest &request, HttpResponse &response)
{
    String separator = "";

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    response.setAllowCrossDomainOrigin("*");
    response.setContentType(ContentType::JSON);
    response.sendString("{\"status\": true,\"available\": [");
    for (int i = 0; i < MAX_MY_SENSORS; i++)
    {
        String sensorStr = String("{\"id\": ") + String(i+1) +
                           String(",\"node\": ") +
                           String(mySensors[i].node) +
                           String(",\"sensor\": ") +
                           String(mySensors[i].sensor) +
                           String(",\"type\": ") +
                           String(mySensors[i].type) +
                           String(",\"value\": \"");
        if (!mySensors[i].value.equals(""))
            sensorStr += String(mySensors[i].value);
        sensorStr += String("\"}");
        response.sendString(separator + sensorStr);
        if (separator.equals(""))
            separator = ",";
    }
    response.sendString("]}");
}

void MyGateway::onWsGetSensors(WebSocket& socket, const String& message)
{
    for (int i = 0; i < MAX_MY_SENSORS; i++)
    {
        socket.sendString(getSensorJson(i));
    }
}

//USAGE: setActuator <node> <sensor> <value>
void MyGateway::onWsSetActuator(WebSocket& socket, const String& message)
{
    Vector<String> commandToken;
    int numToken = splitString((String &)message, ' ' , commandToken);

    if (numToken != 4)
    {
        socket.sendString("{\"status\" : \"error\", \"msg\" : \"invalid number of args\"}");
        return;
    }

    int node = commandToken[1].toInt();
    int sensor = commandToken[2].toInt();
    int value = commandToken[3].toInt();

    for (int idx = 0; idx < MAX_MY_SENSORS; idx++)
    {
        if (mySensors[idx].node == node &&
            mySensors[idx].sensor == sensor)
        {
            MyMessage myMsg;
            myMsg.set(value);
            GW.sendRoute(GW.build(myMsg, node, sensor,
                                  C_SET, 2/* mySensors[idx].type */, 0));
            rfPacketsTx++;
            return;
        }
    }

    socket.sendString("{\"status\" : \"error\", \"msg\" : \"sensor not found\"}");
}

//USAGE: removeSensor <node> <sensor>
void MyGateway::onWsRemoveSensor(WebSocket& socket, const String& message)
{
    Vector<String> commandToken;
    int numToken = splitString((String &)message, ' ' , commandToken);

    if (numToken != 3)
    {
        socket.sendString("{\"status\" : \"error\", \"msg\" : \"invalid number of args\"}");
        return;
    }

    int node = commandToken[1].toInt();
    int sensor = commandToken[2].toInt();

    for (int idx = 0; idx < MAX_MY_SENSORS; idx++)
    {
        if (mySensors[idx].node == node &&
            mySensors[idx].sensor == sensor)
        {
            Debug.printf("Removing sensor %d %d.\n", node, sensor);
            memset(&mySensors[idx], 0, sizeof(sensor_t));
            return;
        }
    }

    socket.sendString("{\"status\" : \"error\", \"msg\" : \"sensor not found\"}");
}

void MyGateway::onRemoveSensor(HttpRequest &request, HttpResponse &response)
{
    String error;

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    JsonObjectStream* stream = new JsonObjectStream();
    JsonObject& json = stream->getRoot();

    String romToRemove = request.getPostParameter("rom");
    if (!romToRemove.equals(""))
    {
        int i = 0;
        char rom[17];

        for (i = 0; i < MAX_MY_SENSORS; i++)
        {
            sprintf(rom, "%d/%d", mySensors[i].node, mySensors[i].sensor);

            if (strcmp(romToRemove.c_str(), rom) == 0)
            {
                Debug.printf("Removing sensor %d with rom %s.\n", i, rom);
                memset(&mySensors[i], 0, sizeof(sensor_t));
                break;
            }
        }

        if (i >= MAX_MY_SENSORS)
        {
            error = "Rom not found";
            goto error;
        }
    }

    json["status"] = (bool)true;
    response.setAllowCrossDomainOrigin("*");
    response.sendJsonObject(stream);
    return;

error:
    json["status"] = (bool)false;
    json.set("error", error);
    response.setAllowCrossDomainOrigin("*");
    response.sendJsonObject(stream);
}

void MyGateway::registerHttpHandlers(HttpServer &server)
{
    server.addPath("/ajax/getSensors", HttpPathDelegate(&MyGateway::onGetSensors, this));
    server.addPath("/ajax/removeSensor", HttpPathDelegate(&MyGateway::onRemoveSensor, this));

    HTTP.addWsCommand("getSensors", WebSocketMessageDelegate(&MyGateway::onWsGetSensors, this));
    HTTP.addWsCommand("setActuator", WebSocketMessageDelegate(&MyGateway::onWsSetActuator, this));
    HTTP.addWsCommand("removeSensor", WebSocketMessageDelegate(&MyGateway::onWsRemoveSensor, this));
}

String MyGateway::getSensorTypeString(int type)
{
    switch (type)
    {
        case 0:
            return "TEMP";
        case 1:
            return "HUM";
        case 2:
            return "LIGHT";
        case 3:
            return "DIMMER";
        case 4:
            return "PRESSURE";
        case 5:
            return "FORECAST";
        case 6:
            return "RAIN";
        case 7:
            return "RAINRATE";
        case 8:
            return "WIND";
        case 9:
            return "GUST";
        case 10:
            return "DIRECTON";
        case 11:
            return "UV";
        case 12:
            return "WEIGHT";
        case 13:
            return "DISTANCE";
        case 14:
            return "IMPEDANCE";
        case 15:
            return "ARMED";
        case 16:
            return "TRIPPED";
        case 17:
            return "WATT";
        case 18:
            return "KWH";
        case 19:
            return "SCENE_ON";
        case 20:
            return "SCENE_OFF";
        case 21:
            return "HEATER";
        case 22:
            return "HEATER_SW";
        case 23:
            return "LIGHT_LEVEL";
        case 24:
            return "VAR1";
        case 25:
            return "VAR2";
        case 26:
            return "VAR3";
        case 27:
            return "VAR4";
        case 28:
            return "VAR5";
        case 29:
            return "UP";
        case 30:
            return "DOWN";
        case 31:
            return "STOP";
        case 32:
            return "IR_SEND";
        case 33:
            return "IR_RECEIVE";
        case 34:
            return "FLOW";
        case 35:
            return "VOLUME";
        case 36:
            return "LOCK_STATUS";
        case 37:
            return "DUST_LEVEL";
        case 38:
            return "VOLTAGE";
        case 39:
            return "CURRENT";
        case 60:
            return "DEFAULT";
        case 61:
            return "SKETCH_NAME";
        case 62:
            return "SKETCH_VERSION";
        case 63:
            return "UNKNOWN";

        default:
            return "";
    }
}

int MyGateway::getSensorTypeFromString(String type)
{
    if (type == "TEMP")
        return 0;
    if (type == "HUM")
        return 1;
    if (type == "LIGHT")
        return 2;
    if (type == "DIMMER")
        return 3;
    if (type == "PRESSURE")
        return 4;
    if (type == "FORECAST")
        return 5;
    if (type == "RAIN")
        return 6;
    if (type == "RAINRATE")
        return 7;
    if (type == "WIND")
        return 8;
    if (type == "GUST")
        return 9;
    if (type == "DIRECTON")
        return 10;
    if (type == "UV")
        return 11;
    if (type == "WEIGHT")
        return 12;
    if (type == "DISTANCE")
        return 13;
    if (type == "IMPEDANCE")
        return 14;
    if (type == "ARMED")
        return 15;
    if (type == "TRIPPED")
        return 16;
    if (type == "WATT")
        return 17;
    if (type == "KWH")
        return 18;
    if (type == "SCENE_ON")
        return 19;
    if (type == "SCENE_OFF")
        return 20;
    if (type == "HEATER")
        return 21;
    if (type == "HEATER_SW")
        return 22;
    if (type == "LIGHT_LEVEL")
        return 23;
    if (type == "VAR1")
        return 24;
    if (type == "VAR2")
        return 25;
    if (type == "VAR3")
        return 26;
    if (type == "VAR4")
        return 27;
    if (type == "VAR5")
        return 28;
    if (type == "UP")
        return 29;
    if (type == "DOWN")
        return 30;
    if (type == "STOP")
        return 31;
    if (type == "IR_SEND")
        return 32;
    if (type == "IR_RECEIVE")
        return 33;
    if (type == "FLOW")
        return 34;
    if (type == "VOLUME")
        return 35;
    if (type == "LOCK_STATUS")
        return 36;
    if (type == "DUST_LEVEL")
        return 37;
    if (type == "VOLTAGE")
        return 38;
    if (type == "CURRENT")
        return 39;
    if (type == "DEFAULT")
        return 60;
    if (type == "SKETCH_NAME")
        return 61;
    if (type == "SKETCH_VERSION")
        return 62;
    if (type == "UNKNOWN")
        return 63;

    return 255;
}

String MyGateway::getSensorValue(String object)
{
    String idStr = object.substring(6);
    int id = idStr.toInt();
    if (id > 0 && id <= MAX_MY_SENSORS)
        return mySensors[id-1].value;
    return "UnknownObjectError";
}

void MyGateway::setSensorValue(String object, String value)
{
    String idStr = object.substring(6);
    int id = idStr.toInt();
    if (id < 1 || id > MAX_MY_SENSORS)
        return;

    MyMessage myMsg;
    myMsg.set(value.c_str());
    GW.sendRoute(GW.build(myMsg, mySensors[id-1].node,
                          mySensors[id-1].sensor, C_SET,
                          2 /*mySensors[id-1].type*/, 0));
    rfPacketsTx++;
}

uint64_t MyGateway::getBaseAddress()
{
    return rfBaseAddress;
}

uint8_t MyGateway::getNumDetectedNodes()
{
    return (numDetectedNodes);
}

uint16_t MyGateway::getNumDetectedSensors()
{
    return (numDetectedSensors);
}

int getRadioStatus ()
{
  return (transport.getRadioStatus());
}

MyGateway GW;

long rfPacketsRx = 0;
long rfPacketsTx = 0;
