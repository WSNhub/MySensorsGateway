#include <user_config.h>
#include <SmingCore.h>
#include <HTTP.h>
#include <Wifi.h>
#include <SDCard.h>
#include <MyGateway.h>
#include <AppSettings.h>
#include <controller.h>

Timer softApSetPasswordTimer;
void apEnable()
{
     Wifi.softApEnable();
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
    if (request.getRequestMethod() == RequestMethod::POST)
    {
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
    
        Wifi.reconnect(500);
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

void onStatus(HttpRequest &request, HttpResponse &response)
{
    char buf [200];
    TemplateFileStream *tmpl = new TemplateFileStream("status.html");
    auto &vars = tmpl->variables();

    vars["ssid"] = AppSettings.ssid;
    vars["wifiStatus"] = isWifiConnected ? "Connected" : "Not connected";
    
    bool dhcp = WifiStation.isEnabledDHCP();
    if (dhcp)
    {
      vars["ipOrigin"] = "From DHCP";
    }
    else
    {
      vars["ipOrigin"] = "Static";
    }

    if (!WifiStation.getIP().isNull())
    {
        vars["ip"] = WifiStation.getIP().toString();
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
    if (AppSettings.useOwnBaseAddress)
      m_snprintf (buf, 200, "%08x (private)", rfBaseAddress);
    else
      m_snprintf (buf, 200, "%08x (default)", rfBaseAddress);
    vars["baseAddress"] = buf;
    vars["radioStatus"] = isNRFAvailable() ? "Available" : "Not available"; //TODO fix
    vars["detNodes"] = GW.getNumDetectedNodes();
    vars["detSensors"] = GW.getNumDetectedSensors();
    
    
    // --- System info -------------------------------------------------
    m_snprintf (buf, 200, "%x", system_get_chip_id());
    vars["systemVersion"] = build_git_sha;
    vars["systemBuild"] = build_time;
    vars["systemFreeHeap"] = system_get_free_heap_size();
    vars["systemStartup"] = "todo";
    vars["systemChipId"] = buf;


    // --- Statistics --------------------------------------------------
    vars["nrfRx"] = rfPacketsRx; //TODO check counters at MySensor.cpp
    vars["nrfTx"] = rfPacketsTx;
    vars["mqttRx"] = mqttPktRx;
    vars["mqttTx"] = mqttPktTx;
    
   response.sendTemplate(tmpl); // will be automatically deleted
}

void onFile(HttpRequest &request, HttpResponse &response)
{
    static bool alreadyInitialized = false;

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
        {
            response.sendFile(file);
        }
    }
}

void HTTPClass::begin()
{
    server.listen(80);
    server.addPath("/", onIpConfig);
    server.addPath("/ipconfig", onIpConfig);
    server.addPath("/status", onStatus);

    GW.registerHttpHandlers(server);
    controller.registerHttpHandlers(server);
    server.setDefaultHandler(onFile);
}

HTTPClass HTTP;
