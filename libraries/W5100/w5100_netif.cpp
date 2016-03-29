#include <SmingCore.h>
#include <user_config.h>

#include "ethernetif.h"
#include "ethernetif_driver.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <lwip/dhcp.h>
err_t etharp_output(struct netif *netif,
                    struct pbuf  *q,
                    ip_addr_t    *ipaddr 
                   );
err_t ethernet_input(struct pbuf  *p,
                     struct netif *netif 
                    );

struct ethernetif w5100_netif_if;
struct netif w5100_netif;

#ifdef __cplusplus
}
#endif

struct ip_addr myip_addr = {0x00000000UL};
struct ip_addr gw_addr   = {0x00000000UL};
struct ip_addr netmask   = {0x00000000UL};

Timer ethTimer;
void ethTimerHandler()
{
    if (netif_default != &w5100_netif)
        netif_set_default(&w5100_netif);

    ethernetif_input(&w5100_netif);
}

void w5100_show_interfaces()
{
    struct netif *netif;
    Serial.printf("\n\nInterfaces:\n");
    for(netif = netif_list; netif != NULL; netif = netif->next) {
        Serial.printf("Found interface %c%c%d\n",
                      netif->name[0], netif->name[1], netif->num);
        IPAddress addr(netif->ip_addr.addr);
        Serial.printf("    IP   %s\n", addr.toString().c_str());
        IPAddress mask(netif->netmask.addr);
        Serial.printf("    MASK %s\n", mask.toString().c_str());
        IPAddress gw(netif->gw.addr);
        Serial.printf("    GW   %s\n", gw.toString().c_str());
    }
}

void w5100_netif_init()
{
    wifi_get_macaddr(STATION_IF, w5100_netif_if.address);
    
    // Add our netif to LWIP (netif_add calls our driver initialization function)
    if (netif_add(&w5100_netif, &myip_addr, &netmask, &gw_addr, &w5100_netif_if,
                  ethernetif_init, ethernet_input) == NULL) {
        Serial.println("netif_add (w5100_netif_init) failed");
    }

    netif_set_default(&w5100_netif);
    //netif_set_up(&w5100_netif);
    dhcp_start(&w5100_netif);

    ethTimer.initializeUs(500, ethTimerHandler).start(true);
}

IPAddress w5100_netif_get_ip()
{
    IPAddress addr(w5100_netif.ip_addr.addr);

    return addr;
}

IPAddress w5100_netif_get_netmask()
{
    IPAddress addr(w5100_netif.netmask.addr);

    return addr;
}

IPAddress w5100_netif_get_gateway()
{
    IPAddress addr(w5100_netif.gw.addr);

    return addr;
}

