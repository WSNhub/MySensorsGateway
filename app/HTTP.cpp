#include <user_config.h>
#include <SmingCore.h>
#include <HTTP.h>
#include <Wifi.h>
#include <SDCard.h>
#include <MyGateway.h>
#include <AppSettings.h>
#include <controller.h>
#include <Services/WebHelpers/base64.h>

Timer softApSetPasswordTimer;
void apEnable()
{
     Wifi.softApEnable();
}

void onIpConfig(HttpRequest &request, HttpResponse &response)
{
    if (!HTTP.isHttpClientAllowed(request, response))
        return;

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

void onFile(HttpRequest &request, HttpResponse &response)
{
    static bool alreadyInitialized = false;

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

void HTTPClass::begin()
{
    server.listen(80);
    server.enableHeaderProcessing("Authorization");
    server.addPath("/", onIpConfig);
    server.addPath("/ipconfig", onIpConfig);

    GW.registerHttpHandlers(server);
    controller.registerHttpHandlers(server);
    server.setDefaultHandler(onFile);
}

HTTPClass HTTP;
