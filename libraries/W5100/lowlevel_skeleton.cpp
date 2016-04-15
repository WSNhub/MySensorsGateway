/**
 * @file
 * low level Ethernet Interface Skeleton
 *
 */

/* portions 
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved. 
 * 
 * See ethernetif.c for copyright details
 */
#include <SmingCore.h>
#include <user_config.h>

#include <stdint.h>
#include "ethernetif.h"

#include "w5100.h"

SOCKET s; // our socket that will be opened in RAW mode

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
void
low_level_init(void *i, uint8_t *addr, void *mcast)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    uint8_t ip_address[4] = { 0, 0, 0, 0 };
    uint8_t mac_address[6] = {0, 0, 0, 0, 0, 0};
    W5100.init();
    SPI.beginTransaction(SPI_ETHERNET_SETTINGS);
    W5100.setMACAddress(mac_address);
    W5100.setIPAddress(ip_address);
    SPI.endTransaction();
    W5100.writeSnMR(s, SnMR::MACRAW); 
    W5100.execCmdSn(s, Sock_OPEN);
    Serial.println("W5100 initialized.");
#endif
}

/**
 * This function starts the transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param ethernetif the lwip network interface structure for this netif
 * @return 1 if the packet could be sent
 *         0 if the packet couldn't be sent	(no space on chip)
 */

int
low_level_startoutput(void *i)
{
  //Serial.println("===> low_level_startoutput");
  return 1;
}

/**
 * This function should do the actual transmission of the packet.
 The packet is contained in chained pbufs, so this function will be called
 for each chunk
 *
 * @param ethernetif the lwip network interface structure for this netif
 * @param data where the data is
 * @param len the block size
 */

void
low_level_output(void *i, void *data, uint16_t len)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    //Serial.println("===> low_level_output");
    W5100.send_data_processing(s, (uint8_t *)data, len);
    //for (int i=0; i<len; i++)
    //{
    //    uint8_t*arr = (uint8_t*)data;
    //    Serial.print(arr[i], HEX);
    //    Serial.print(" ");  
    //}
    //Serial.print("\n");
#endif
}

/**
 * This function begins the actual transmission of the packet, ending the process
 *
 * @param ethernetif the lwip network interface structure for this netif
 */

void
low_level_endoutput(void *i, uint16_t total_len)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    //Serial.println("===> low_level_endoutput");
    W5100.execCmdSn(s, Sock_SEND_MAC);
#endif	
}
/**
 * This function checks for a packet on the chip, and returns its length
 * @param ethernetif the lwip network interface structure for this netif
 * @return 0 if no packet, packet length otherwise
 */
int
low_level_startinput(void *i)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    uint8_t header[8];
    int rxLen;
    int pktLen;

    /*
     * Check the size of the RX buffer, It should hold at least the Wiznet
     * header (2B) and a minimum ethernet header (14B).
     */
    rxLen = W5100.getRXReceivedSize(s);
    if (rxLen < 16)
        return 0;

    //Serial.printf("buffer: %d bytes\n", rxLen);

    /* Peek the header added by the Wiznet chip (2B) and the DST MAC (6B) */
    W5100.recv_data_processing(s, header, 8, true);

    /* Calculate the length of the packet we're going to retrieve */
    pktLen = (header[0] << 8) + header[1];

    /* Dump length and DST MAC
    Serial.printf("header: %d [%02x %02x]\n", pktLen, header[0], header[1]);
    Serial.printf("DST MAC: %02x:%02x:%02x:%02x:%02x:%02x\n",
                  header[2], header[3], header[4],
                  header[5], header[6], header[7]);*/

    /*
     * If the packet is maller than 16 bytes there is something wrong. At
     * least 16 bytes need to be present (2B Wiznet header, 14B minimal
     * ethernet header.
     * When the packet exceeds the MTU by more than 16 bytes, there is also
     * a problem with the packet.
     * In both cases treat the whole buffer as a packet hoping next rx will
     * be a sane one again.
     */
    if (pktLen < 16 ||
        pktLen > (2 + 14 + ETHERNET_MTU))
    {
        /* Probably corrupted, treat buffer as one packet */
        W5100.recv_data_processing(s, header, 2);
        W5100.execCmdSn(s, Sock_RECV);
        Serial.printf("RX (corrupt?) packet: %d bytes, buffer %d bytes\n",
                      pktLen - 2, rxLen - 2);
        return rxLen - 2;
    }

    if (rxLen >= pktLen)
    {
        /* The whole packet is in the buffer */
        W5100.recv_data_processing(s, header, 2);
        W5100.execCmdSn(s, Sock_RECV);
        //Serial.printf("RX packet: %d bytes\n", pktLen - 2);
        return pktLen - 2;
    }

    /* The packet is not fully in the buffer, process whatever is available */
    W5100.recv_data_processing(s, header, 2);
    W5100.execCmdSn(s, Sock_RECV);
    Serial.printf("RX (incomplete?) packet: %d bytes\n", rxLen - 2);
    return rxLen - 2;
#endif
}

/**
 * This function takes the data from the chip and copies it to a chained pbuf
 * @param ethernetif the lwip network interface structure for this netif
 * @param data where the data is
 * @param len the block size
 */
void
low_level_input(void *i, void *data, uint16_t len)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    W5100.recv_data_processing(s, (uint8_t*)data, len);
    W5100.execCmdSn(s, Sock_RECV);

    /*Serial.printf("INPUT: %d bytes, read into %x\n",
                  len, (unsigned int)(data));
    for (int i=0; i<len; i++)
    {
        uint8_t*arr = (uint8_t*)data;
        Serial.print(arr[i], HEX);
        Serial.print(" ");  
    }
    Serial.print("\n");*/
#endif 
}

/**
 * This function ends the receive process
 * @param ethernetif the lwip network interface structure for this netif
 */
void
low_level_endinput(void *i)
{
#if WIRED_ETHERNET_MODE != WIRED_ETHERNET_NONE
    //Serial.println("===> low_level_endinput");
    //W5100.execCmdSn(s, Sock_RECV);
#endif
}

/**
 * This function is called in case there is not enough memory to hold a frame
 * after its length has been got from the chip. The driver decides whether to 
 * drop it or let it waiting in the chip's memory, based on the developer's
 * knowledge of the hardware (the chip can have more or less memory than the system)
 * @param ethernetif the lwip network interface structure for this netif
 * @param len the frame length
 */
void
low_level_input_nomem(void *i, uint16_t len)
{
    Serial.println("===> low_level_input_nomem");
}

