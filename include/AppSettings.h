/*
 * AppSettings.h
 *
 *  Created on: 13 мая 2015 г.
 *      Author: Anakod
 */

#include <SmingCore/SmingCore.h>

#ifndef INCLUDE_APPSETTINGS_H_
#define INCLUDE_APPSETTINGS_H_

#define APP_SETTINGS_FILE ".settings.conf" // leading point for security reasons :)

struct ApplicationSettingsStorage
{
    String ssid;
    String password;
    String apPassword;

    String portalUrl;
    String portalData;

    bool dhcp = true;

    IPAddress ip;
    IPAddress netmask;
    IPAddress gateway;

    String mqttUser;
    String mqttPass;
    String mqttServer;
    int    mqttPort;
    String mqttSensorPfx;
    String mqttControllerPfx;

    bool   cpuBoost = true;
    bool   useOwnBaseAddress = false;

    String cloudDeviceToken;
    String cloudLogin;
    String cloudPassword;

    void load()
    {
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

            delete[] jsonString;
        }
    }

    void save()
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

        network["dhcp"] = dhcp;

        // Make copy by value for temporary string objects
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

        //TODO: add direct file stream writing
        String out;
        root.printTo(out);
        fileSetContent(APP_SETTINGS_FILE, out);
    }

    bool exist() { return fileExist(APP_SETTINGS_FILE); }
};

static ApplicationSettingsStorage AppSettings;

#endif /* INCLUDE_APPSETTINGS_H_ */
