#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Network/TelnetServer.h>
#include <AppSettings.h>
#include <globals.h>
#include <i2c.h>
#include <CloudController.h>
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
MyI2C I2C_dev;

HttpServer server;
TelnetServer telnet;
static boolean first_time = TRUE;

char convBuf[MAX_PAYLOAD*2+1];

#ifndef DISABLE_SPIFFS
MyInterpreter interpreter;
#endif

CloudController *controller = &cloudController;

#ifdef __cplusplus
extern "C" {
#endif
#include "mutex.h"
#ifdef __cplusplus
}
#endif

class MyMutex
{
  public:
    MyMutex()
    {
        CreateMutux(&mutex);
    };
    void Lock()
    {
        while (!GetMutex(&mutex))
            delay(0);
    };
    void Unlock()
    {
        ReleaseMutex(&mutex);
    };
  private:
    mutex_t mutex;
};

MyMutex interpreterMutex;

void incomingMessage(const MyMessage &message)
{
    // Pass along the message from sensors to serial line
    Debug.printf("APP RX %d;%d;%d;%d;%d;%s\n",
                  message.sender, message.sensor,
                  mGetCommand(message), mGetAck(message),
                  message.type, message.getString(convBuf));

    if (mGetCommand(message) == C_SET)
    {
        String type = gw.getSensorTypeString(message.type);
        String topic = message.sender + String("/") +
                       message.sensor + String("/") +
                       "V_" + type;
        controller->notifyChange(topic, message.getString(convBuf));
    }

#ifndef DISABLE_SPIFFS
    interpreterMutex.Lock();
    interpreter.setVariable('n', message.sender);
    interpreter.setVariable('s', message.sensor);
    interpreter.setVariable('v', atoi(message.getString(convBuf)));
    interpreter.run();
    interpreterMutex.Unlock();
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
}

Timer softApSetPasswordTimer;
void softApSetPassword()
{
    WifiAccessPoint.enable(false);
    WifiAccessPoint.enable(true);

    if (AppSettings.apPassword.equals(""))
    {
	WifiAccessPoint.config("MySensors gateway", "", AUTH_OPEN);
    }
    else
    {
	WifiAccessPoint.config("MySensors gateway",
			       AppSettings.apPassword,
			       AUTH_WPA_WPA2_PSK);
    }
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
    if (request.getRequestMethod() == RequestMethod::POST)
    {
        String oldApPass = AppSettings.apPassword;
        AppSettings.apPassword = request.getPostParameter("apPassword");
        if (!AppSettings.apPassword.equals(oldApPass))
        {
            softApSetPasswordTimer.initializeMs(10, softApSetPassword).startOnce();
        }

        AppSettings.ssid = request.getPostParameter("ssid");
        AppSettings.password = request.getPostParameter("password");
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
        interpreterMutex.Lock();
        interpreter.loadFile((char *)".rules");
        interpreterMutex.Unlock();
    }
#endif

    TemplateFileStream *tmpl = new TemplateFileStream("rules.html");
    auto &vars = tmpl->variables();

#ifndef DISABLE_SPIFFS
    interpreterMutex.Lock();
    if (fileExist(".rules"))
    {
        vars["rule"] = fileGetContent(".rules");        
    }
    else
    {
        vars["rule"] = "";
    }
    interpreterMutex.Unlock();
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
    controller->registerHttpHandlers(server);
    server.setDefaultHandler(onFile);
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

uint64_t rfBaseAddress;

// Will be called when system initialization was completed
void startServers()
{
    char id[16];

    // Start AP for configuration
    WifiAccessPoint.enable(true);
    sprintf(id, "%x", system_get_chip_id());

    if (AppSettings.apPassword.equals(""))
    {
        WifiAccessPoint.config((String)"MySensors gateway " + id,
                               "", AUTH_OPEN);
    }
    else
    {
        WifiAccessPoint.config((String)"MySensors gateway " + id,
                               AppSettings.apPassword, AUTH_WPA_WPA2_PSK);
    }

    wasConnected = FALSE;
    connectionCheckTimer.initializeMs(1000,
                                      wifiCheckState).start(true);

    startWebServer();
    telnet.listen(23);

    interpreterMutex.Lock();
    interpreter.registerFunc1((char *)"print", print);
    interpreter.registerFunc3((char *)"updateSensorState", updateSensorState);
    interpreter.loadFile((char *)".rules");
    interpreterMutex.Unlock();

    if (AppSettings.useOwnBaseAddress)
    {
        rfBaseAddress = ((uint64_t)system_get_chip_id()) << 8;
    }
    else
    {
        rfBaseAddress = RF24_BASE_RADIO_ID;
    }

    controller->begin();
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
    if (first_time) 
    {
      first_time = FALSE;
      // start getting sensor data
      gw.begin(incomingMessage, NULL, rfBaseAddress);
    }
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
    out->printf("System information : ESP8266 based MySensors gateway\r\n");
    out->printf("Build time         : %s\n", build_time);
    out->printf("Version            : %s\n", build_git_sha);
    out->printf("Sming Version      : 2.1.1\r\n");
    out->printf("ESP SDK version    : %s\n", system_get_sdk_version());
    out->printf("\r\n");
    out->printf("Time               : ");
    out->printf(SystemClock.getSystemTimeString().c_str());
    out->printf("\r\n");
    out->printf("Free Heap          : %d\r\n", system_get_free_heap_size());
    out->printf("CPU Frequency      : %d MHz\r\n", system_get_cpu_freq());
    out->printf("System Chip ID     : %x\r\n", system_get_chip_id());
    out->printf("SPI Flash ID       : %x\r\n", spi_flash_get_id());
    out->printf("SPI Flash Size     : %d\r\n", (1 << ((spi_flash_get_id() >> 16) & 0xff)));
    out->printf("\r\n");
    out->printf("RF base address    : %02x", (rfBaseAddress >> 32) & 0xff);
    out->printf("%08x\r\n", rfBaseAddress);
    out->printf("\r\n");
}

void processRestartCommand(String commandLine, CommandOutput* out)
{
    System.restart();
}

void processDebugCommand(String commandLine, CommandOutput* out)
{
    Vector<String> commandToken;
    int numToken = splitString(commandLine, ' ' , commandToken);

    if (numToken != 2 ||
        (commandToken[1] != "on" && commandToken[1] != "off"))
    {
        out->printf("usage : \r\n\r\n");
        out->printf("debug on  : Enable debug\r\n");
        out->printf("debug off : Disable debug\r\n");
        return;
    }

    if (commandToken[1] == "on")
        Debug.start();
    else
        Debug.stop();
}

void processCpuCommand(String commandLine, CommandOutput* out)
{
    Vector<String> commandToken;
    int numToken = splitString(commandLine, ' ' , commandToken);

    if (numToken != 2 ||
        (commandToken[1] != "80" && commandToken[1] != "160"))
    {
        out->printf("usage : \r\n\r\n");
        out->printf("cpu 80  : Run at 80MHz\r\n");
        out->printf("cpu 160 : Run at 160MHz\r\n");
        return;
    }

    if (commandToken[1] == "80")
    {
        System.setCpuFrequency(eCF_80MHz);
        AppSettings.cpuBoost = false;
    }
    else
    {
        System.setCpuFrequency(eCF_160MHz);
        AppSettings.cpuBoost = true;
    }

    AppSettings.save();
}

void processBaseAddressCommand(String commandLine, CommandOutput* out)
{
    Vector<String> commandToken;
    int numToken = splitString(commandLine, ' ' , commandToken);

    if (numToken != 2 ||
        (commandToken[1] != "default" && commandToken[1] != "private"))
    {
        out->printf("usage : \r\n\r\n");
        out->printf("base-address default : Use the default MySensors base address\r\n");
        out->printf("base-address private : Use a base address based on ESP chip ID\r\n");
        return;
    }

    if (commandToken[1] == "default")
    {
        AppSettings.useOwnBaseAddress = false;
    }
    else
    {
        AppSettings.useOwnBaseAddress = true;
    }

    AppSettings.save();
    System.restart();
}

void processShowConfigCommand(String commandLine, CommandOutput* out)
{
    out->println(fileGetContent(".settings.conf"));
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
    Serial.commandProcessing(true);
    Debug.start();

    // set prompt
    commandHandler.setCommandPrompt("MySensorsGateway > ");

    // add new commands
    commandHandler.registerCommand(CommandDelegate("info",
                                                   "Show system information",
                                                   "System",
                                                   processInfoCommand));
    commandHandler.registerCommand(CommandDelegate("debug",
                                                   "Enable or disable debugging",
                                                   "System",
                                                   processDebugCommand));
    commandHandler.registerCommand(CommandDelegate("restart",
                                                   "Restart the system",
                                                   "System",
                                                   processRestartCommand));
    commandHandler.registerCommand(CommandDelegate("cpu",
                                                   "Adjust CPU speed",
                                                   "System",
                                                   processCpuCommand));
    commandHandler.registerCommand(CommandDelegate("debug",
                                                   "Enable/disable debugging",
                                                   "System",
                                                   processDebugCommand));
    commandHandler.registerCommand(CommandDelegate("showConfig",
                                                   "Show the current configuration",
                                                   "System",
                                                   processShowConfigCommand));
    commandHandler.registerCommand(CommandDelegate("base-address",
                                                   "Set the base address to use",
                                                   "MySensors",
                                                   processBaseAddressCommand));

    AppSettings.load();

    I2C_dev.begin();

    WifiStation.enable(true);
    // why not ?
    WifiAccessPoint.enable(true);

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

    // CPU boost
    if (AppSettings.cpuBoost)
        System.setCpuFrequency(eCF_160MHz);
    else
        System.setCpuFrequency(eCF_80MHz);

    // Run WEB server on system ready
    System.onReady(startServers);
}
