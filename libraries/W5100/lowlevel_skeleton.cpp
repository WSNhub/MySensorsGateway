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
    uint8_t dummy[2];
    int rbuflen;

    rbuflen = W5100.getRXReceivedSize(s);
    if (rbuflen < 2)
    {
        return 0;
    }
    else
    {
        W5100.recv_data_processing(s, dummy, 2); //first 2 bytes = length
        W5100.execCmdSn(s, Sock_RECV);
        //Serial.printf("incoming: %d bytes\n", rbuflen - 2);
        return rbuflen - 2;
    }
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
    //Serial.printf("INPUT: %d bytes, read into %x\n", len, (unsigned int)(data));
    //for (int i=0; i<len; i++)
    //{
    //    uint8_t*arr = (uint8_t*)data;
    //    Serial.print(arr[i], HEX);
    //    Serial.print(" ");  
    //}
    //Serial.print("\n");
    W5100.execCmdSn(s, Sock_RECV);
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
    W5100.execCmdSn(s, Sock_RECV);
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

