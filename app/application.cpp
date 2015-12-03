#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <AppSettings.h>
#include "Libraries/MySensors/MyGateway.h"
#include "Libraries/MySensors/MyTransport.h"
#include "Libraries/MySensors/MyTransportNRF24.h"
#include "Libraries/MySensors/MyHwESP8266.h"
#include "Libraries/MyInterpreter/MyInterpreter.h"

#define RADIO_CE_PIN        2   // radio chip enable
#define RADIO_SPI_SS_PIN    15  // radio SPI serial select
MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);
MyHwESP8266 hw;
MyGateway gw(transport, hw);

HttpServer server;
FTPServer ftp;

BssList networks;
String network, password;
Timer connectionTimer;

void SendToSensor(String message)
{
    MyMessage msg;
    gw.sendRoute(gw.build(msg, 22, 1, C_REQ, 2, 0));
}

char convBuf[MAX_PAYLOAD*2+1];

MyInterpreter interpreter;

void mqttPublishMessage(String topic, String message);

void incomingMessage(const MyMessage &message)
{
    // Pass along the message from sensors to serial line
    Serial.printf("APP RX %d;%d;%d;%d;%d;%s\n",
                  message.sender, message.sensor,
                  mGetCommand(message), mGetAck(message),
                  message.type, message.getString(convBuf));

    if (mGetCommand(message)!=C_PRESENTATION)
    {
        String topic = message.sender + String("/") +
                       message.sensor + String("/");
        #ifdef MQTT_TRANSLATE_TYPES
        //V_?
        topic += getType(convBuf, &vType[message.type]);
        #else
        topic += message.type;
        #endif
        mqttPublishMessage(topic, message.getString(convBuf));
    }

    interpreter.setVariable('n', message.sender);
    interpreter.setVariable('s', message.sensor);
    interpreter.setVariable('v', atoi(message.getString(convBuf)));

#ifndef DISABLE_SPIFFS
    if (fileExist("rules.script"))
    {
        int size = fileGetSize("rules.script");
        char* progBuf = new char[size + 1];
        fileGetContent("rules.script", progBuf, size + 1);
        interpreter.run(progBuf, strlen(progBuf));
        delete progBuf;
    }
#else
    return;
#endif
}

void onIndex(HttpRequest &request, HttpResponse &response)
{
	TemplateFileStream *tmpl = new TemplateFileStream("index.html");
	auto &vars = tmpl->variables();
	response.sendTemplate(tmpl); // will be automatically deleted
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
	if (request.getRequestMethod() == RequestMethod::POST)
	{
		AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
		AppSettings.ip = request.getPostParameter("ip");
		AppSettings.netmask = request.getPostParameter("netmask");
		AppSettings.gateway = request.getPostParameter("gateway");
		debugf("Updating IP settings: %d", AppSettings.ip.isNull());
		AppSettings.save();
	}

	TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
	auto &vars = tmpl->variables();

	bool dhcp = WifiStation.isEnabledDHCP();
	vars["dhcpon"] = dhcp ? "checked='checked'" : "";
	vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

	if (!WifiStation.getIP().isNull())
	{
		vars["ip"] = WifiStation.getIP().toString();
		vars["netmask"] = WifiStation.getNetworkMask().toString();
		vars["gateway"] = WifiStation.getNetworkGateway().toString();
	}
	else
	{
		vars["ip"] = "192.168.1.77";
		vars["netmask"] = "255.255.255.0";
		vars["gateway"] = "192.168.1.1";
	}

	response.sendTemplate(tmpl); // will be automatically deleted
}

void onFile(HttpRequest &request, HttpResponse &response)
{
	String file = request.getPath();
	if (file[0] == '/')
		file = file.substring(1);

	if (file[0] == '.')
		response.forbidden();
	else
	{
		response.setCache(86400, true); // It's important to use cache for better performance.
		response.sendFile(file);
	}
}

void onAjaxNetworkList(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	json["status"] = (bool)true;

	bool connected = WifiStation.isConnected();
	json["connected"] = connected;
	if (connected)
	{
		// Copy full string to JSON buffer memory
		json.addCopy("network", WifiStation.getSSID());
	}

	JsonArray& netlist = json.createNestedArray("available");
	for (int i = 0; i < networks.count(); i++)
	{
		if (networks[i].hidden) continue;
		JsonObject &item = netlist.createNestedObject();
		item.add("id", (int)networks[i].getHashId());
		// Copy full string to JSON buffer memory
		item.addCopy("title", networks[i].ssid);
		item.add("signal", networks[i].rssi);
		item.add("encryption", networks[i].getAuthorizationMethodName());
	}

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

void makeConnection()
{
	WifiStation.enable(true);
	WifiStation.config(network, password);

	AppSettings.ssid = network;
	AppSettings.password = password;
	AppSettings.save();

	network = ""; // task completed
}

void onAjaxConnect(HttpRequest &request, HttpResponse &response)
{
	JsonObjectStream* stream = new JsonObjectStream();
	JsonObject& json = stream->getRoot();

	String curNet = request.getPostParameter("network");
	String curPass = request.getPostParameter("password");

	bool updating = curNet.length() > 0 && (WifiStation.getSSID() != curNet || WifiStation.getPassword() != curPass);
	bool connectingNow = WifiStation.getConnectionStatus() == eSCS_Connecting || network.length() > 0;

	if (updating && connectingNow)
	{
		debugf("wrong action: %s %s, (updating: %d, connectingNow: %d)", network.c_str(), password.c_str(), updating, connectingNow);
		json["status"] = (bool)false;
		json["connected"] = (bool)false;
	}
	else
	{
		json["status"] = (bool)true;
		if (updating)
		{
			network = curNet;
			password = curPass;
			debugf("CONNECT TO: %s %s", network.c_str(), password.c_str());
			json["connected"] = false;
			connectionTimer.initializeMs(1200, makeConnection).startOnce();
		}
		else
		{
			json["connected"] = WifiStation.isConnected();
			debugf("Network already selected. Current status: %s", WifiStation.getConnectionStatusName());
		}
	}

	if (!updating && !connectingNow && WifiStation.isConnectionFailed())
		json["error"] = WifiStation.getConnectionStatusName();

	response.setAllowCrossDomainOrigin("*");
	response.sendJsonObject(stream);
}

extern void checkMqttClient();
void onMqttConfig(HttpRequest &request, HttpResponse &response)
{
    AppSettings.load();
    if (request.getRequestMethod() == RequestMethod::POST)
    {
        AppSettings.mqttUser = request.getPostParameter("user");
        AppSettings.mqttPass = request.getPostParameter("password");
        AppSettings.mqttServer = request.getPostParameter("server");
        AppSettings.mqttPort = atoi(request.getPostParameter("port").c_str());
        AppSettings.save();
        if (WifiStation.isConnected())
            checkMqttClient();
    }

    TemplateFileStream *tmpl = new TemplateFileStream("mqtt.html");
    auto &vars = tmpl->variables();

    vars["user"] = AppSettings.mqttUser;
    vars["password"] = AppSettings.mqttPass;
    vars["server"] = AppSettings.mqttServer;
    vars["port"] = AppSettings.mqttPort;

    response.sendTemplate(tmpl); // will be automatically deleted
}

void startWebServer()
{
	server.listen(80);
	server.addPath("/", onIndex);
	server.addPath("/ipconfig", onIpConfig);
	server.addPath("/mqttconfig", onMqttConfig);
	server.addPath("/ajax/get-networks", onAjaxNetworkList);
	server.addPath("/ajax/connect", onAjaxConnect);
        gw.registerHttpHandlers(server);
	server.setDefaultHandler(onFile);
}

void startFTP()
{
	if (!fileExist("index.html"))
		fileSetContent("index.html", "<h3>Please connect to FTP and upload files from folder 'web/build' (details in code)</h3>");

	// Start FTP server
	ftp.listen(21);
	ftp.addUser("me", "123"); // FTP account
}

int print(int a)
{
  Serial.println(a);
  return a;
}

int updateSensorState(int node, int sensor, int value)
{
  MyMessage myMsg;
  myMsg.set(value);
  gw.sendRoute(gw.build(myMsg, node, sensor, C_SET, 2 /*message.type*/, 0));
}

// Will be called when system initialization was completed
void startServers()
{
    startFTP();
    startWebServer();

    interpreter.registerFunc1((char *)"print", print);
    interpreter.registerFunc3((char *)"updateSensorState", updateSensorState);

    gw.begin(incomingMessage);
}

void networkScanCompleted(bool succeeded, BssList list)
{
    if (succeeded)
    {
        for (int i = 0; i < list.count(); i++)
            if (!list[i].hidden && list[i].ssid.length() > 0)
                networks.add(list[i]);
    }
    networks.sort([](const BssInfo& a, const BssInfo& b){ return b.rssi - a.rssi; } );
}

Timer connectionCheckTimer;
bool wasConnected = FALSE;
void connectOk();
void connectFail();

void wifiCheckState()
{
    if (WifiStation.isConnected())
    {
        if (!wasConnected)
        {
            Serial.println("CONNECTED");
            wasConnected = TRUE;
            connectOk();
        }
        checkMqttClient();
    }
    else
    {
        if (wasConnected)
        {
            Serial.println("NOT CONNECTED");
            wasConnected = FALSE;
            connectFail();
        }
    }
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
    Serial.println("--> I'm CONNECTED");
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
    Serial.println("--> I'm NOT CONNECTED. Need help :(");
}

void processApplicationCommands(String commandLine, CommandOutput* commandOutput)
{
    Vector<String> commandToken;
    int numToken = splitString(commandLine, ' ' , commandToken);

    if (numToken == 1)
    {
        commandOutput->printf("Example subcommands available : \r\n");
    }
    commandOutput->printf("This command is handled by the application\r\n");
}

void init()
{
    /* Mount the internal storage */
    int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
    if (slot == 0)
    {
#ifdef RBOOT_SPIFFS_0
        Serial.printf("trying to mount spiffs at %x, length %d\n", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
        spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
        Serial.printf("trying to mount spiffs at %x, length %d\n", 0x40300000, SPIFF_SIZE);
        spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
    }
    else
    {
#ifdef RBOOT_SPIFFS_1
        Serial.printf("trying to mount spiffs at %x, length %d\n", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
        spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
        Serial.printf("trying to mount spiffs at %x, length %d\n", 0x40500000, SPIFF_SIZE);
        spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
    }
#else
    Serial.println("spiffs disabled");
#endif

    Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
    Serial.systemDebugOutput(true); // Enable debug output to serial
    //commandHandler.registerCommand(CommandDelegate("example","Example Command from Class","Application",processApplicationCommands));
    //Serial.commandProcessing(true);

    AppSettings.load();

    WifiStation.enable(true);
    //WifiStation.config("WSNatWork", "");
    if (AppSettings.exist())
    {
        WifiStation.config(AppSettings.ssid, AppSettings.password);
        if (!AppSettings.dhcp && !AppSettings.ip.isNull())
            WifiStation.setIP(AppSettings.ip, AppSettings.netmask, AppSettings.gateway);
    }
    WifiStation.startScan(networkScanCompleted);

    // Start AP for configuration
    WifiAccessPoint.enable(true);
    WifiAccessPoint.config("MySensors gateway", "", AUTH_OPEN);

    wasConnected = FALSE;
    connectionCheckTimer.initializeMs(1000, wifiCheckState).start(true);

    // Run WEB server on system ready
    System.onReady(startServers);
}
