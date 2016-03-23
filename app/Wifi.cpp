#include <user_config.h>
#include <SmingCore/SmingCore.h>
#include <SmingCore/Debug.h>
#include <globals.h>
#include <AppSettings.h>
#include "Wifi.h"
#include "RTClock.h"

void
wifi_cb ( System_Event_t *e )
{
    Wifi.handleEvent(e);
}

extern void otaEnable();

void WifiClass::begin(WifiStateChangeDelegate dlg)
{
    WifiStation.enable(false);

    changeDlg = dlg;

    if (AppSettings.apMode == apModeAlwaysOn ||
        AppSettings.apMode == apModeWhenDisconnected)
    {
        softApEnable();
    }
    else
    {
        softApDisable();        
    }

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
    /*else
    {
        String SSID = WifiStation.getSSID();
        if (!SSID.equals(""))
        {
            AppSettings.ssid = SSID;
            AppSettings.password = WifiStation.getPassword();
            AppSettings.save();
        }
    }*/

    wifi_set_event_handler_cb(wifi_cb);

    if (AppSettings.ssid.equals(""))
    {
        WifiStation.enable(false);
    }
    else
    {
        reconnect(1);
    }

    otaEnable();    
}

void WifiClass::softApEnable()
{
    char id[16];

    if (AppSettings.apMode == apModeAlwaysOff ||
        AppSettings.apMode == apModeWhenDisconnected && connected)
    {
        Debug.println("Not enabling AP due to config setting");
        return;
    }

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

void WifiClass::softApDisable()
{
    if (AppSettings.apMode == apModeAlwaysOn ||
        AppSettings.apMode == apModeWhenDisconnected && !connected)
    {
        Debug.println("Not disabling AP due to config setting");
        return;
    }

    WifiAccessPoint.enable(false);
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
        {
            connected = true;

            Debug.printf("Wifi client got connected\n");
            ntpClient.requestTime();

            // Disable SoftAP.
            // This function will check whether the AP can be disabled.
            softApDisable();
        }
        connected = true;
    }
    else if (event == EVENT_STAMODE_DISCONNECTED)
    {
        static int  numApNotFound = 0;
        static int  numOther = 0;

        if (e->event_info.disconnected.reason == REASON_NO_AP_FOUND)
        {
            numOther = 0;
            numApNotFound++;
            if (numApNotFound == 10)
            {
	        wifi_station_disconnect();
                reconnect(60000);
                numApNotFound = 0;
            }
        }
        else
        {
            numApNotFound = 0;
        }

        if (connected)
        {
            connected = false;
            Debug.printf("Wifi client got disconnected (%d)\n",
                         e->event_info.disconnected.reason);
            // Enable SoftAP.
            // This function will check whether the AP can be enabled.
            softApEnable();
        }

        connected = false;

        if (changeDlg)
            changeDlg(false);
    }
}

void WifiClass::portalLoginHandler(HttpClient& client, bool successful)
{
    String response = client.getResponseString();
    Debug.println("Portal server response: '" + response + "'");
}

void WifiClass::connect()
{
    Debug.println("Connecting...");

    if (!WifiStation.getSSID().equals(AppSettings.ssid) ||
        !WifiStation.getPassword().equals(AppSettings.password))
    {
        WifiStation.config(AppSettings.ssid, AppSettings.password,
                           FALSE);
    }

    if (!AppSettings.dhcp && !AppSettings.ip.isNull())
    {
        WifiStation.setIP(AppSettings.ip,
                          AppSettings.netmask,
                          AppSettings.gateway);
    }

    WifiStation.enable(true);
    wifi_station_connect();
    if (AppSettings.dhcp)
        wifi_station_dhcpc_start();
    wifi_station_set_reconnect_policy(true);
}

void WifiClass::reconnect(int delayMs)
{
    reconnectTimer.initializeMs(delayMs, TimerDelegate(&WifiClass::connect, this)).startOnce();
}

void WifiClass::ntpTimeResultHandler(NtpClient& client, time_t ntpTime)
{
    SystemClock.setTime(ntpTime, eTZ_UTC);
    Debug.print("Time after NTP sync: ");
    Debug.println(SystemClock.getSystemTimeString());
    Clock.setTime(ntpTime);
}

WifiClass Wifi;
