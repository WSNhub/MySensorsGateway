/*
 * Author: Laurent Carlier <carlier.lau@gmail.com>
 */

#ifndef INCLUDE_MDNS_H_
#define INCLUDE_MDNS_H_

#include <SmingCore/SmingCore.h>

class mDNS;

struct DNS_packet_t;

class mDNS: public UdpConnection
{
public:
  mDNS();
  void begin();
  static const String LOCAL_DOMAIN_NAME;
  virtual ~mDNS();
protected:
  void onReceive(pbuf* buf, IPAddress remoteIP, uint16_t remotePort);

private:
  mDNS(const mDNS& orig);
  void startmDNS();
  bool isRequestValid(char *data, int size, IPAddress remoteIP, uint16_t remotePort);
  bool isRequestAQuery(DNS_packet_t* aPacket, uint16_t size);
  void processQuery(const DNS_packet_t *aPacket, uint16_t size);
  void sendAnswer(uint16_t pktId, const char *currentHostName);
  void sendNotImplemented(DNS_packet_t* aPacket, uint16_t size);

  static unsigned int buildAnswerPacket(unsigned char* packetBuf, uint16_t pktId, const char *hostname);
  static unsigned int readDnsName(char *bufferToFillin, const unsigned char *packetBuf, const unsigned char* ptrToData);

  static const unsigned int PORT_NR;
  static const IPAddress ANSWER_IP;
  static struct netif *findNetIf(IPAddress &addr);
  static bool joinIgmpGroup();
  static struct ip_addr multicast_addr;
  static struct ip_addr host_addr;

  Timer connectionTimer;
};

#endif

