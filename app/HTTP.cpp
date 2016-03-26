#include <user_config.h>
#include <SmingCore.h>
#include <HTTP.h>
#include <Network.h>
#include <SDCard.h>
#include <MyGateway.h>
#include <AppSettings.h>
#include <controller.h>
#include <Services/WebHelpers/base64.h>
#include <Wiring/SplitString.h>

// Forward declarations
void StartOtaUpdateWeb(String);
void processRestartCommandWeb(void);


Timer softApSetPasswordTimer;
void apEnable()
{
     Network.softApEnable();
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    if (request.getRequestMethod() == RequestMethod::POST)
    {
        bool connectionTypeChanges =
        	AppSettings.wired != (request.getPostParameter("wired") == "1");

        AppSettings.wired = request.getPostParameter("wired") == "1";

        String oldApPass = AppSettings.apPassword;
        AppSettings.apPassword = request.getPostParameter("apPassword");
        if (!AppSettings.apPassword.equals(oldApPass))
        {
            softApSetPasswordTimer.initializeMs(500, apEnable).startOnce();
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
    
        Network.reconnect(500);

        if (connectionTypeChanges)
            processRestartCommandWeb();
    }

#if WIRED_ETHERNET_MODE == WIRED_ETHERNET_NONE
    TemplateFileStream *tmpl = new TemplateFileStream("settings.html");
#else
    TemplateFileStream *tmpl = new TemplateFileStream("settings_wired.html");
#endif
    auto &vars = tmpl->variables();

    vars["wiredon"] = AppSettings.wired ? "checked='checked'" : "";
    vars["wiredoff"] = AppSettings.wired ? "" : "checked='checked'";

    vars["ssid"] = AppSettings.ssid;
    vars["password"] = AppSettings.password;
    vars["apPassword"] = AppSettings.apPassword;

    vars["portalUrl"] = AppSettings.portalUrl;
    vars["portalData"] = AppSettings.portalData;

    bool dhcp = AppSettings.dhcp;
    vars["dhcpon"] = dhcp ? "checked='checked'" : "";
    vars["dhcpoff"] = !dhcp ? "checked='checked'" : "";

    vars["ip"] = Network.getClientIP().toString();
    vars["netmask"] = Network.getClientMask().toString();
    vars["gateway"] = Network.getClientGW().toString();

    response.sendTemplate(tmpl); // will be automatically deleted
}

void onStatus(HttpRequest &request, HttpResponse &response)
{
    char buf [200];
    TemplateFileStream *tmpl = new TemplateFileStream("status.html");
    auto &vars = tmpl->variables();

    vars["ssid"] = AppSettings.ssid;
    vars["wifiStatus"] = isNetworkConnected ? "Connected" : "Not connected";
    
    bool dhcp = AppSettings.dhcp;
    if (dhcp)
    {
      vars["ipOrigin"] = "From DHCP";
    }
    else
    {
      vars["ipOrigin"] = "Static";
    }

    if (!Network.getClientIP().isNull())
    {
        vars["ip"] = Network.getClientIP().toString();
    }
    else
    {
        vars["ip"] = "0.0.0.0";
        vars["ipOrigin"] = "not configured";
    }
    if (AppSettings.mqttServer != "")
    {
        vars["mqttIp"] = AppSettings.mqttServer;
        vars["mqttStatus"] = isMqttConnected() ? "Connected":"Not connected";
    }
    else
    {
        vars["mqttIp"] = "0.0.0.0";
        vars["mqttStatus"] = "Not configured";
    }
    uint64_t rfBaseAddress = GW.getBaseAddress();
    uint32_t rfBaseLow = (rfBaseAddress & 0xffffffff);
    uint8_t  rfBaseHigh = ((rfBaseAddress >> 32) & 0xff);
    if (AppSettings.useOwnBaseAddress)
      sprintf (buf, "%02x%08x (private)", rfBaseHigh, rfBaseLow);
    else
      sprintf (buf, "%02x%08x (default)", rfBaseHigh, rfBaseLow);
    vars["baseAddress"] = buf;
    vars["radioStatus"] = getRadioStatus();
    vars["detNodes"] = GW.getNumDetectedNodes();
    vars["detSensors"] = GW.getNumDetectedSensors();
    
    
    // --- System info -------------------------------------------------
    sprintf (buf, "%x", system_get_chip_id());
    vars["systemVersion"] = build_git_sha;
    vars["systemBuild"] = build_time;
    vars["systemFreeHeap"] = system_get_free_heap_size();
    vars["systemStartup"] = "todo";
    vars["systemChipId"] = buf;
    uint32_t curMillis = millis();
    sprintf(buf, "%d s, %03d ms : ", curMillis/1000, curMillis % 1000);
    vars["systemUptime"] = buf;


    // --- Statistics --------------------------------------------------
    vars["nrfRx"] = rfPacketsRx; //TODO check counters at MySensor.cpp
    vars["nrfTx"] = rfPacketsTx;
    vars["mqttRx"] = mqttPktRx;
    vars["mqttTx"] = mqttPktTx;
    
   response.sendTemplate(tmpl); // will be automatically deleted
}

void onMaintenance(HttpRequest &request, HttpResponse &response)
{
    AppSettings.load();

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    if (request.getRequestMethod() == RequestMethod::POST)
    {
        String command = request.getPostParameter("Command");
        
        if (command.equals("Upgrade")) {
            AppSettings.webOtaBaseUrl = request.getPostParameter("webOtaBaseUrl");

            AppSettings.save();

            Debug.println("Going to call: StartOtaUpdateWeb()");
            StartOtaUpdateWeb(AppSettings.webOtaBaseUrl);
            Debug.println("Called: StartOtaUpdateWeb()");
        }
        else if (command.equals("Restart")) {
            Debug.println("Going to call: processRestartCommandWeb()");
            processRestartCommandWeb();
            Debug.println("Called: processRestartCommandWeb()");
        }
        else {
            Debug.printf("Unknown command: [%s]\r\n", command.c_str());
        }

    }

    TemplateFileStream *tmpl = new TemplateFileStream("maintenance.html");
    auto &vars = tmpl->variables();

    vars["webOtaBaseUrl"] = AppSettings.webOtaBaseUrl;

    response.sendTemplate(tmpl); // will be automatically deleted
}

void onFile(HttpRequest &request, HttpResponse &response)
{
#ifdef SD_SPI_SS_PIN
    static bool alreadyInitialized = false;
#endif

    if (!HTTP.isHttpClientAllowed(request, response))
        return;

    String file = request.getPath();
    if (file[0] == '/')
        file = file.substring(1);

    if (file[0] == '.')
        response.forbidden();
    else
    {
        response.setCache(86400, true); // It's important to use cache for better performance.

        // open the file. note that only one file can be open at a time,
        // so you have to close this one before opening another.
        Debug.printf("REQUEST for %s\n", file.c_str());
#ifdef SD_SPI_SS_PIN
        if (alreadyInitialized || SD.begin(0))
        {
            alreadyInitialized = true;
            Debug.printf("SD card present\n");
            File f = SD.open(file);
            if (!f) //SD.exists(file) does not seem to work
            {
                Debug.printf("NOT FOUND %s\n", file.c_str());
                response.sendFile(file);
            }
            else if (f.isDirectory())
            {
                f.close();
                Debug.printf("%s IS A DIRECTORY\n", file.c_str());
                response.forbidden();
            }
            else
            {
                f.close();
                Debug.printf("OPEN STREAM FOR %s\n", file.c_str());
                response.setAllowCrossDomainOrigin("*");
                const char *mime = ContentType::fromFullFileName(file);
		if (mime != NULL)
		    response.setContentType(mime);
                SdFileStream *stream = new SdFileStream(file);
                response.sendDataStream(stream);
            }
        }
        else
#endif
        {
            response.sendFile(file);
        }
    }
}

bool HTTPClass::isHttpClientAllowed(HttpRequest &request, HttpResponse &response)
{
    if (AppSettings.apPassword.equals(""))
        return true;

    String authHeader = request.getHeader("Authorization");
    char userpass[32+1+32+1];
    if (!authHeader.equals("") && authHeader.startsWith("Basic"))
    {
        Debug.printf("Authorization header %s\n", authHeader.c_str());
        int r = base64_decode(authHeader.length()-6,
                              authHeader.substring(6).c_str(),
                              sizeof(userpass),
                              (unsigned char *)userpass);
        if (r > 0)
        {
            userpass[r]=0; //zero-terminate user:pass string
            Debug.printf("Authorization header decoded %s\n", userpass);
            String validUserPass = "admin:"+AppSettings.apPassword;
            if (validUserPass.equals(userpass))
            {
                return true;
            }
        }
    }

    response.authorizationRequired();
    response.setHeader("Content-Type", "text/plain");
    response.setHeader("WWW-Authenticate",
                       String("Basic realm=\"MySensors Gateway ") + system_get_chip_id() + "\"");
    return false;
}

void HTTPClass::wsConnected(WebSocket& socket)
{
    //
}

void HTTPClass::wsMessageReceived(WebSocket& socket, const String& message)
{
    Vector<String> commandToken;
    int numToken = splitString((String &)message,' ' , commandToken);

    if (numToken > 0 && wsCommandHandlers.contains(commandToken[0]))
    {
        Serial.printf("WebSocket command received: %s\r\n", commandToken[0].c_str());
        wsCommandHandlers[commandToken[0]](socket, message);
        return;
    }
    else
    {
        Serial.printf("WebSocket received unknown string: %s\r\n", message.c_str());
        String response = "Unknown command: " + message;
        socket.sendString(response);
    }
}

void HTTPClass::wsBinaryReceived(WebSocket& socket, uint8_t* data, size_t size)
{
	Serial.printf("Websocket binary data recieved, size: %d\r\n", size);
}

void HTTPClass::wsDisconnected(WebSocket& socket)
{
    //
}

void HTTPClass::addWsCommand(String command, WebSocketMessageDelegate callback)
{
    debugf("'%s' registered", command.c_str());
    wsCommandHandlers[command] = callback;
}

void HTTPClass::notifyWsClients(String message)
{
    WebSocketsList &clients = server.getActiveWebSockets();
    for (int i = 0; i < clients.count(); i++)
        clients[i].sendString(message);
}

void HTTPClass::begin()
{
    server.listen(80);
    server.enableHeaderProcessing("Authorization");
    server.addPath("/", onStatus);
    server.addPath("/ipconfig", onIpConfig);
    server.addPath("/status", onStatus);
    server.addPath("/maintenance", onMaintenance);

    GW.registerHttpHandlers(server);
    controller.registerHttpHandlers(server);
    server.setDefaultHandler(onFile);

    // Web Sockets configuration
    server.enableWebSockets(true);
    server.setWebSocketConnectionHandler(
        WebSocketDelegate(&HTTPClass::wsConnected, this));
    server.setWebSocketMessageHandler(
        WebSocketMessageDelegate(&HTTPClass::wsMessageReceived, this));
    server.setWebSocketBinaryHandler(
        WebSocketBinaryDelegate(&HTTPClass::wsBinaryReceived, this));
    server.setWebSocketDisconnectionHandler(
        WebSocketDelegate(&HTTPClass::wsDisconnected, this));
}

HTTPClass HTTP;
