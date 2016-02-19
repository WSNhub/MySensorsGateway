#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>
#include <AppSettings.h>
#include "Wifi.h"

void
wifi_cb ( System_Event_t *e )
{
    Wifi.handleEvent(e);
}

extern void otaEnable();

void WifiClass::begin(WifiStateChangeDelegate dlg)
{
    changeDlg = dlg;

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

    wifi_set_event_handler_cb(wifi_cb);

    WifiStation.enable(true);
    softApEnable();

    otaEnable();    
}

void WifiClass::softApEnable()
{
    char id[16];

    WifiAccessPoint.enable(false);

    // Start AP for configuration
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

    WifiAccessPoint.enable(true);
}

void WifiClass::handleEvent(System_Event_t *e)
{
    int event = e->event;

    if (event == EVENT_STAMODE_GOT_IP)
    {
	Debug.printf("Wifi client got IP\n");
        if (!haveIp && changeDlg)
            changeDlg(true);
        haveIp = true;

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
                url, HttpClientCompletedDelegate(&WifiClass::portalLoginHandler, this));
        }
    }
    else if (event == EVENT_STAMODE_CONNECTED)
    {
	if (!connected)
            Debug.printf("Wifi client got connected\n");
        connected = true;
    }
    else if (event == EVENT_STAMODE_DISCONNECTED)
    {
	if (connected)
            Debug.printf("Wifi client got disconnected\n");
        connected = false;
        if (changeDlg)
            changeDlg(false);
    }
    else
    {
	Debug.printf("Unknown wifi event %d !\n", event);
    }
}

void WifiClass::portalLoginHandler(HttpClient& client, bool successful)
{
    String response = client.getResponseString();
    Debug.println("Portal server response: '" + response + "'");
}

WifiClass Wifi;
