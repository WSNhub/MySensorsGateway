/*
 * AppSettings.h
 *
 *  Created on: 13 ��� 2015 �.
 *      Author: Anakod
 */

#include <SmingCore/SmingCore.h>
#include <AppSettings.h>

ApplicationSettingsStorage::ApplicationSettingsStorage(): dataLoaded(false)
{
}


void ApplicationSettingsStorage::load()
{
    if(dataLoaded)
    {
        return;
    }
    char strChipId[32];
    String defaultHostName("swig_");
    m_snprintf(strChipId, 32, "%#0x", system_get_chip_id());
    defaultHostName += strChipId;

    DynamicJsonBuffer jsonBuffer;
    if (exist())
    {
        int size = fileGetSize(APP_SETTINGS_FILE);
        char* jsonString = new char[size + 1];
        fileGetContent(APP_SETTINGS_FILE, jsonString, size + 1);
        JsonObject& root = jsonBuffer.parseObject(jsonString);

        JsonObject& network = root["network"];

        ssid = (const char *)network["ssid"];
        password = (const char *)network["password"];
        apPassword = (const char *)network["apPassword"];
        portalUrl = (const char *)network["portalUrl"];
        portalData = (const char *)network["portalData"];

        if (!network.containsKey("apMode"))
        {
            apMode = apModeAlwaysOn;
        }
        else
        {
            String newMode = (const char *)network["apMode"];
            if (newMode.equals("always"))
                apMode = apModeAlwaysOn;
            else if (newMode.equals("never"))
                apMode = apModeAlwaysOff;
            else if (newMode.equals("whenDisconnected"))
                apMode = apModeWhenDisconnected;
            else
                apMode = apModeAlwaysOff;
        }

        if (!network.containsKey(HOSTNAME_KEY))
        {
          hostname = defaultHostName;
        }
        else
        {
          hostname = (const char*)network[HOSTNAME_KEY];
        }

        dhcp = network["dhcp"];

        ip = (const char *)network["ip"];
        netmask = (const char *)network["netmask"];
        gateway = (const char *)network["gateway"];

        JsonObject& mqtt = root["mqtt"];
        mqttUser = (const char *)mqtt["user"];
        mqttPass = (const char *)mqtt["password"];
        mqttServer = (const char *)mqtt["server"];
        mqttPort = mqtt["port"];
        mqttSensorPfx = (const char *)mqtt["sensorPfx"];
        mqttControllerPfx = (const char *)mqtt["controllerPfx"];

        cpuBoost = root["cpuBoost"];
        useOwnBaseAddress = root["useOwnBaseAddress"];

        cloudDeviceToken = (const char *)root["cloudDeviceToken"];
        cloudLogin = (const char *)root["cloudLogin"];
        cloudPassword = (const char *)root["cloudPassword"];

        webOtaBaseUrl = (const char *)root["webOtaBaseUrl"];

        delete[] jsonString;
    }
    else
    {
        hostname = defaultHostName;
    }
    dataLoaded = true;
}

void ApplicationSettingsStorage::save()
{
    DynamicJsonBuffer jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();

    JsonObject& network = jsonBuffer.createObject();
    root["network"] = network;
    network["ssid"] = ssid.c_str();
    network["password"] = password.c_str();
    network["apPassword"] = apPassword.c_str();
    network["portalUrl"] = portalUrl.c_str();
    network["portalData"] = portalData.c_str();

    if (apMode == apModeAlwaysOn)
        network["apMode"] = "always";
    else if (apMode == apModeAlwaysOff)
        network["apMode"] = "never";
    else
        network["apMode"] = "whenDisconnected";

    network["dhcp"] = dhcp;

    // Make copy by value for temporary string objects
    network.set(HOSTNAME_KEY, hostname.c_str());
    network.set("ip", ip.toString());
    network.set("netmask", netmask.toString());
    network.set("gateway", gateway.toString());

    JsonObject& mqtt = jsonBuffer.createObject();
    root["mqtt"] = mqtt;
    mqtt.set("user", mqttUser);
    mqtt.set("password", mqttPass);
    mqtt.set("server", mqttServer);
    mqtt["port"] = mqttPort;
    mqtt.set("sensorPfx", mqttSensorPfx);
    mqtt.set("controllerPfx", mqttControllerPfx);

    root["cpuBoost"] = cpuBoost;
    root["useOwnBaseAddress"] = useOwnBaseAddress;

    root.set("cloudDeviceToken", cloudDeviceToken);
    root.set("cloudLogin", cloudLogin);
    root.set("cloudPassword", cloudPassword);

    root.set("webOtaBaseUrl", webOtaBaseUrl.c_str());

    //TODO: add direct file stream writing
    String out;
    root.printTo(out);
    fileSetContent(APP_SETTINGS_FILE, out);
}

ApplicationSettingsStorage AppSettings;
