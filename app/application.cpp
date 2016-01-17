#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <AppSettings.h>
#include <mqtt.h>
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
TelnetServer telnet;

char convBuf[MAX_PAYLOAD*2+1];

#ifndef DISABLE_SPIFFS
MyInterpreter interpreter;
#endif

void mqttPublishMessage(String topic, String message);

char V_0[] = "TEMP";        //V_TEMP
char V_1[] = "HUM";        //V_HUM
char V_2[] = "LIGHT";        //V_LIGHT
char V_3[] = "DIMMER";        //V_DIMMER
char V_4[] = "PRESSURE";    //V_PRESSURE
char V_5[] = "FORECAST";    //V_FORECAST
char V_6[] = "RAIN";        //V_RAIN
char V_7[] = "RAINRATE";    //V_RAINRATE
char V_8[] = "WIND";        //V_WIND
char V_9[] = "GUST";        //V_GUST
char V_10[] = "DIRECTON";    //V_DIRECTON
char V_11[] = "UV";        //V_UV
char V_12[] = "WEIGHT";        //V_WEIGHT
char V_13[] = "DISTANCE";    //V_DISTANCE
char V_14[] = "IMPEDANCE";    //V_IMPEDANCE
char V_15[] = "ARMED";        //V_ARMED
char V_16[] = "TRIPPED";    //V_TRIPPED
char V_17[] = "WATT";        //V_WATT
char V_18[] = "KWH";        //V_KWH
char V_19[] = "SCENE_ON";    //V_SCENE_ON
char V_20[] = "SCENE_OFF";    //V_SCENE_OFF
char V_21[] = "HEATER";        //V_HEATER
char V_22[] = "HEATER_SW";    //V_HEATER_SW
char V_23[] = "LIGHT_LEVEL";    //V_LIGHT_LEVEL
char V_24[] = "VAR1";        //V_VAR1
char V_25[] = "VAR2";        //V_VAR2
char V_26[] = "VAR3";        //V_VAR3
char V_27[] = "VAR4";        //V_VAR4
char V_28[] = "VAR5";        //V_VAR5
char V_29[] = "UP";        //V_UP
char V_30[] = "DOWN";        //V_DOWN
char V_31[] = "STOP";        //V_STOP
char V_32[] = "IR_SEND";    //V_IR_SEND
char V_33[] = "IR_RECEIVE";    //V_IR_RECEIVE
char V_34[] = "FLOW";        //V_FLOW
char V_35[] = "VOLUME";        //V_VOLUME
char V_36[] = "LOCK_STATUS";    //V_LOCK_STATUS
char V_37[] = "DUST_LEVEL";    //V_DUST_LEVEL
char V_38[] = "VOLTAGE";    //V_VOLTAGE
char V_39[] = "CURRENT";    //V_CURRENT
char V_40[] = "";        //
char V_41[] = "";        //
char V_42[] = "";        //
char V_43[] = "";        //
char V_44[] = "";        //
char V_45[] = "";        //
char V_46[] = "";        //
char V_47[] = "";        //
char V_48[] = "";        //
char V_49[] = "";        //
char V_50[] = "";        //
char V_51[] = "";        //
char V_52[] = "";        //
char V_53[] = "";        //
char V_54[] = "";        //
char V_55[] = "";        //
char V_56[] = "";        //
char V_57[] = "";        //
char V_58[] = "";        //
char V_59[] = "";        //
char V_60[] = "DEFAULT";    //Custom for MQTTGateway
char V_61[] = "SKETCH_NAME";    //Custom for MQTTGateway
char V_62[] = "SKETCH_VERSION"; //Custom for MQTTGateway
char V_63[] = "UNKNOWN";     //Custom for MQTTGateway

//////////////////////////////////////////////////////////////////

const char *vType[] = {
    V_0, V_1, V_2, V_3, V_4, V_5, V_6, V_7, V_8, V_9, V_10,
    V_11, V_12, V_13, V_14, V_15, V_16, V_17, V_18, V_19, V_20,
    V_21, V_22, V_23, V_24, V_25, V_26, V_27, V_28, V_29, V_30,
    V_31, V_32, V_33, V_34, V_35, V_36, V_37, V_38, V_39, V_40,
    V_41, V_42, V_43, V_44, V_45, V_46, V_47, V_48, V_49, V_50,
    V_51, V_52, V_53, V_54, V_55, V_56, V_57, V_58, V_59, V_60,
    V_61, V_62, V_63
};

int getTypeFromString(String type)
{
    for (int x = 0; x < 64; x++)
    {
        if (type.substring(2).equals(vType[x]))
        {
            return x;
        }
    }

    return 0;
}

void incomingMessage(const MyMessage &message)
{
    // Pass along the message from sensors to serial line
    Debug.printf("APP RX %d;%d;%d;%d;%d;%s\n",
                  message.sender, message.sensor,
                  mGetCommand(message), mGetAck(message),
                  message.type, message.getString(convBuf));

    if (mGetCommand(message) == C_SET)
    {
        const char *type = vType[message.type];
        String topic = message.sender + String("/") +
                       message.sensor + String("/") +
                       "V_" + type;
        mqttPublishMessage(topic, message.getString(convBuf));
    }

#ifndef DISABLE_SPIFFS
    noInterrupts();
    interpreter.setVariable('n', message.sender);
    interpreter.setVariable('s', message.sensor);
    interpreter.setVariable('v', atoi(message.getString(convBuf)));
    interpreter.run();
    interrupts();
#endif

    return;
}

Timer reconnectTimer;
void wifiConnect()
{
    WifiStation.enable(false);
    WifiStation.enable(true);

    if (AppSettings.ssid.equals("") &&
        !WifiStation.getSSID().equals(""))
    {
        AppSettings.ssid = WifiStation.getSSID();
        AppSettings.password = WifiStation.getPassword();
        AppSettings.save();
    }

    WifiStation.config(AppSettings.ssid, AppSettings.password);
    if (!AppSettings.dhcp && !AppSettings.ip.isNull())
    {
        WifiStation.setIP(AppSettings.ip,
                          AppSettings.netmask,
                          AppSettings.gateway);
    }

    WifiAccessPoint.enable(true);
    if (AppSettings.apPassword.equals(""))
    {
        WifiAccessPoint.config("MySensors gateway", "", AUTH_OPEN);
    }
    else
    {
        WifiAccessPoint.config("MySensors gateway",
                               AppSettings.apPassword, AUTH_WPA_WPA2_PSK);
    }
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
    if (request.getRequestMethod() == RequestMethod::POST)
    {
        AppSettings.ssid = request.getPostParameter("ssid");
        AppSettings.password = request.getPostParameter("password");
        AppSettings.apPassword = request.getPostParameter("apPassword");
        AppSettings.portalUrl = request.getPostParameter("portalUrl");
        AppSettings.portalData = request.getPostParameter("portalData");

        AppSettings.dhcp = request.getPostParameter("dhcp") == "1";
        AppSettings.ip = request.getPostParameter("ip");
        AppSettings.netmask = request.getPostParameter("netmask");
        AppSettings.gateway = request.getPostParameter("gateway");
        Debug.printf("Updating IP settings: %d", AppSettings.ip.isNull());
        AppSettings.save();
    
        reconnectTimer.initializeMs(10, wifiConnect).startOnce();
    }

    TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
    auto &vars = tmpl->variables();

    vars["ssid"] = AppSettings.ssid;
    vars["password"] = AppSettings.password;
    vars["apPassword"] = AppSettings.apPassword;

    vars["portalUrl"] = AppSettings.portalUrl;
    vars["portalData"] = AppSettings.portalData;

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
        vars["ip"] = "0.0.0.0";
        vars["netmask"] = "255.255.255.255";
        vars["gateway"] = "0.0.0.0";
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

void onRules(HttpRequest &request, HttpResponse &response)
{
#ifndef DISABLE_SPIFFS
    if (request.getRequestMethod() == RequestMethod::POST)
    {
	fileSetContent(".rules",
                       request.getPostParameter("rule"));
        noInterrupts();
        interpreter.loadFile((char *)".rules");
        interrupts();
    }
#endif

    TemplateFileStream *tmpl = new TemplateFileStream("rules.html");
    auto &vars = tmpl->variables();

#ifndef DISABLE_SPIFFS
    noInterrupts();
    if (fileExist(".rules"))
    {
        vars["rule"] = fileGetContent(".rules");        
    }
    else
    {
        vars["rule"] = "";
    }
    interrupts();
#else
    vars["rule"] = "";
#endif

    response.sendTemplate(tmpl); // will be automatically deleted
}

void startWebServer()
{
    server.listen(80);
    server.addPath("/", onIpConfig);
    server.addPath("/ipconfig", onIpConfig);
    server.addPath("/rules.html", onRules);

    gw.registerHttpHandlers(server);
    mqttRegisterHttpHandlers(server);
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
  Debug.println(a);
  return a;
}

int updateSensorStateInt(int node, int sensor, int type, int value)
{
  MyMessage myMsg;
  myMsg.set(value);
  gw.sendRoute(gw.build(myMsg, node, sensor, C_SET, type, 0));
}

int updateSensorState(int node, int sensor, int value)
{
  updateSensorStateInt(node, sensor, 2 /*message.type*/, value);
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
            Debug.println("CONNECTED");
            wasConnected = TRUE;
            connectOk();
        }
        checkMqttClient();
    }
    else
    {
        if (wasConnected)
        {
            Debug.println("NOT CONNECTED");
            wasConnected = FALSE;
            connectFail();
        }
    }
}


// Will be called when system initialization was completed
void startServers()
{
    // Start AP for configuration
    WifiAccessPoint.enable(true);
    if (AppSettings.apPassword.equals(""))
    {
        WifiAccessPoint.config("MySensors gateway", "", AUTH_OPEN);
    }
    else
    {
        WifiAccessPoint.config("MySensors gateway",
                               AppSettings.apPassword, AUTH_WPA_WPA2_PSK);
    }

    wasConnected = FALSE;
    connectionCheckTimer.initializeMs(1000,
                                      wifiCheckState).start(true);

    startFTP();
    startWebServer();
    telnet.listen(23);
    //telnet.setDebug(true);

    noInterrupts();
    interpreter.registerFunc1((char *)"print", print);
    interpreter.registerFunc3((char *)"updateSensorState", updateSensorState);
    interpreter.loadFile((char *)".rules");
    interrupts();

    gw.begin(incomingMessage);
}

HttpClient portalLogin;

void onActivateDataSent(HttpClient& client, bool successful)
{
    String response = client.getResponseString();
    Debug.println("Server response: '" + response + "'");
}

// Will be called when WiFi station was connected to AP
void connectOk()
{
    Debug.println("--> I'm CONNECTED");
    if (WifiStation.getIP().isNull())
    {
        Debug.println("No ip?");
        return;
    }

    if (!AppSettings.portalUrl.equals(""))
    {
        String mac;
        uint8 hwaddr[6] = {0};
        wifi_get_macaddr(STATION_IF, hwaddr);
        for (int i = 0; i < 6; i++)
        {
            if (hwaddr[i] < 0x10) mac += "0";
                mac += String(hwaddr[i], HEX);
            if (i < 5) mac += ":";
        }

        String body = AppSettings.portalData;
        body.replace("{ip}", WifiStation.getIP().toString());
        body.replace("{mac}", mac);
        portalLogin.setPostBody(body.c_str());
        String url = AppSettings.portalUrl;
        url.replace("{ip}", WifiStation.getIP().toString());
        url.replace("{mac}", mac);

        portalLogin.downloadString(
            url, HttpClientCompletedDelegate(onActivateDataSent));
    }
}

// Will be called when WiFi station timeout was reached
void connectFail()
{
    Debug.println("--> I'm NOT CONNECTED. Need help :(");
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

void processInfoCommand(String commandLine, CommandOutput* out)
{
    out->printf("\r\n");
    //out->printf("Version        : %s\n", build_git_sha);
    //out->printf("Build time     : %s\n", build_git_time);
    out->printf("Free Heap      : %d\r\n", system_get_free_heap_size());
    out->printf("CPU Frequency  : %d MHz\r\n", system_get_cpu_freq());
    out->printf("System Chip ID : %x\r\n", system_get_chip_id());
    out->printf("SPI Flash ID   : %x\r\n", spi_flash_get_id());
    out->printf("SPI Flash Size : %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
    out->printf("\r\n");
}

void processRestartCommand(String commandLine, CommandOutput* out)
{
    System.restart();
}

void processDebugCommand(String commandLine, CommandOutput* out)
{
    Debug.start();
}

void processNoDebugCommand(String commandLine, CommandOutput* out)
{
    Debug.stop();
}

void processBoostonCommand(String commandLine, CommandOutput* out)
{
    System.setCpuFrequency(eCF_160MHz);
}

void processBoostoffCommand(String commandLine, CommandOutput* out)
{
    System.setCpuFrequency(eCF_80MHz);
}

extern void otaEnable();

void init()
{
    /* Mount the internal storage */
    int slot = rboot_get_current_rom();
#ifndef DISABLE_SPIFFS
    if (slot == 0)
    {
#ifdef RBOOT_SPIFFS_0
        Debug.printf("trying to mount spiffs at %x, length %d\n", RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
        spiffs_mount_manual(RBOOT_SPIFFS_0 + 0x40200000, SPIFF_SIZE);
#else
        Debug.printf("trying to mount spiffs at %x, length %d\n", 0x40300000, SPIFF_SIZE);
        spiffs_mount_manual(0x40300000, SPIFF_SIZE);
#endif
    }
    else
    {
#ifdef RBOOT_SPIFFS_1
        Debug.printf("trying to mount spiffs at %x, length %d\n", RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
        spiffs_mount_manual(RBOOT_SPIFFS_1 + 0x40200000, SPIFF_SIZE);
#else
        Debug.printf("trying to mount spiffs at %x, length %d\n", 0x40500000, SPIFF_SIZE);
        spiffs_mount_manual(0x40500000, SPIFF_SIZE);
#endif
    }
#else
    Debug.println("spiffs disabled");
#endif

    Serial.begin(SERIAL_BAUD_RATE); // 115200 by default
    Serial.systemDebugOutput(true); // Enable debug output to serial
    Debug.start();

    //commandHandler.registerCommand(CommandDelegate("example","Example Command from Class","Application",processApplicationCommands));
    commandHandler.registerCommand(CommandDelegate("info",
                                                   "Show system information",
                                                   "System",
                                                   processInfoCommand));
    commandHandler.registerCommand(CommandDelegate("restart",
                                                   "Restart the system",
                                                   "System",
                                                   processRestartCommand));
    commandHandler.registerCommand(CommandDelegate("NOdebug",
                                                   "Disable debugging",
                                                   "System",
                                                   processNoDebugCommand));
    commandHandler.registerCommand(CommandDelegate("debug",
                                                   "Enable debugging",
                                                   "System",
                                                   processDebugCommand));
    commandHandler.registerCommand(CommandDelegate("booston",
                                                   "CPU at 160MHz",
                                                   "System",
                                                   processBoostonCommand));
    commandHandler.registerCommand(CommandDelegate("boostoff",
                                                   "CPU at 80MHz",
                                                   "System",
                                                   processBoostoffCommand));
    Serial.commandProcessing(true);

    AppSettings.load();

    WifiStation.enable(true);
    if (AppSettings.exist())
    {
        if (AppSettings.ssid.equals("") &&
            !WifiStation.getSSID().equals(""))
        {
            AppSettings.ssid = WifiStation.getSSID();
            AppSettings.password = WifiStation.getPassword();
            AppSettings.save();
        }

        WifiStation.config(AppSettings.ssid, AppSettings.password);
        if (!AppSettings.dhcp && !AppSettings.ip.isNull())
        {
            WifiStation.setIP(AppSettings.ip,
                              AppSettings.netmask,
                              AppSettings.gateway);
        }
    }
    else
    {
        String SSID = WifiStation.getSSID();
        if (!SSID.equals(""))
        {
            AppSettings.ssid = SSID;
            AppSettings.password = WifiStation.getPassword();
            AppSettings.save();
        }
    }

    otaEnable();

    // boost
    System.setCpuFrequency(eCF_160MHz);

    // Run WEB server on system ready
    System.onReady(startServers);
}
