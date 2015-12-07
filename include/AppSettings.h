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
			ssid = network["ssid"].toString();
			password = network["password"].toString();
                        portalUrl = network["portalUrl"].toString();
			portalData = network["portalData"].toString();

			dhcp = network["dhcp"];

			ip = network["ip"].toString();
			netmask = network["netmask"].toString();
			gateway = network["gateway"].toString();

                        JsonObject& mqtt = root["mqtt"];
			mqttUser = mqtt["user"].toString();
			mqttPass = mqtt["password"].toString();
			mqttServer = mqtt["server"].toString();
			mqttPort = mqtt["port"];

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
		network["portalUrl"] = portalUrl.c_str();
		network["portalData"] = portalData.c_str();

		network["dhcp"] = dhcp;

		// Make copy by value for temporary string objects
		network.addCopy("ip", ip.toString());
		network.addCopy("netmask", netmask.toString());
		network.addCopy("gateway", gateway.toString());

                JsonObject& mqtt = jsonBuffer.createObject();
		root["mqtt"] = mqtt;
                mqtt.addCopy("user", mqttUser);
                mqtt.addCopy("password", mqttPass);
                mqtt.addCopy("server", mqttServer);
                mqtt["port"] = mqttPort;

		//TODO: add direct file stream writing
		fileSetContent(APP_SETTINGS_FILE, root.toJsonString());
	}

	bool exist() { return fileExist(APP_SETTINGS_FILE); }
};

static ApplicationSettingsStorage AppSettings;

#endif /* INCLUDE_APPSETTINGS_H_ */
