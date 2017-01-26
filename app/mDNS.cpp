/*
 * Author: Laurent Carlier <carlier.lau@gmail.com>
 */
#include "mDNS.h"
#include "Network.h"
#include "AppSettings.h"
#include <cstdint>
#include <cstdio>
#include <SmingCore/SmingCore.h>
#include <Wiring/WConstants.h>
#include <lwip/app/espconn.h>

const IPAddress mDNS::ANSWER_IP(224, 0, 0, 251);
const unsigned int mDNS::PORT_NR=5353;
const String mDNS::LOCAL_DOMAIN_NAME(".local");

struct ip_addr mDNS::host_addr = {0};
struct ip_addr mDNS::multicast_addr = {0};

static u8_t mdns_netif_client_id;
static struct udp_pcb *mdns_pcb;

#define MAX_HOSTNAME_SIZE (255) //From RFC 1035
#define MAX_USER_HOSTNAME_SIZE (MAX_HOSTNAME_SIZE - LOCAL_DOMAIN_NAME.length() - 1) //-1 because the request contains a leading '.'
#define MAX_PACKET_SIZE (1500)
#define DEFAULT_TTL (120)

//rfc1035: Message compression
#define IS_MESSAGE_COMPRESSION(byte) ((byte & 0xc0) == 0xc0)

#define GET_MESSATE_COMPRESSION_OFFSET(byte) (byte & 0x3fff)

#define SIZEOF_DNS_PACKET_HEADER (sizeof(DNS_packet_t) - sizeof(void *))

PACK_STRUCT_BEGIN
struct DNS_flags_t
{
#if BYTE_ORDER == LITTLE_ENDIAN
  uint8_t RD : 1;
  uint8_t TC : 1;
  uint8_t AA : 1;
  uint8_t Opcode : 4;
  uint8_t QR : 1;

  uint8_t Rcode : 4;
  uint8_t CD : 1;
  uint8_t AD : 1;
  uint8_t Z : 1;
  uint8_t RA : 1;
#elif BYTE_ORDER == BIG_ENDIAN
  uint8_t QR : 1;
  uint8_t Opcode : 4;
  uint8_t AA : 1;
  uint8_t TC : 1;
  uint8_t RD : 1;

  uint8_t RA : 1;
  uint8_t Z : 1;
  uint8_t AD : 1;
  uint8_t CD : 1;
  uint8_t Rcode : 4;
#else
#error "Byte order not defined"
#endif
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct DNS_Query_t
{
  //name var length
  uint16_t type;
  uint16_t qclass;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

#define SIZEOF_DNS_QUERY(sizeOfNameField) (sizeof(DNS_Query_t) + sizeOfNameField)

#define SIZEOF_DNS_RR(answerName, rdata_length) (strlen(answerName) + 1 + sizeof(DNS_RR_t) - sizeof(void *) + rdata_length) // +1 for \0

PACK_STRUCT_BEGIN
struct DNS_RR_class_t
{
#if BYTE_ORDER == LITTLE_ENDIAN
  uint16_t qclass:15;
  uint16_t flush : 1;
#elif BYTE_ORDER == BIG_ENDIAN
  uint16_t flush : 1;
  uint16_t query_class:15;
#else
#error "Byte order not defined"
#endif
    /*};
    uint16_t raw;
  };*/
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct DNS_RR_t
{
  //name var length
  uint16_t type;
  uint16_t qclass;
  //DNS_RR_class_t query;
  uint32_t TTL;
  uint16_t rdata_length;
  unsigned char *rdata;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

PACK_STRUCT_BEGIN
struct DNS_packet_t
{
  uint16_t id;
  DNS_flags_t flags;
  uint16_t totalQuestions;
  uint16_t totalAnswerRRs;
  uint16_t totalAuthorityRRs;
  uint16_t totalAdditionalRRs;
  unsigned char *data;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

const uint16_t DNS_class_Reserved = 0;
const uint16_t DNS_class_Internet = 1;
const uint16_t DNS_class_Flush = 0x8000;
const uint16_t DNS_class_Chaos = 3;
const uint16_t DNS_class_Hesiod = 4;
const uint16_t DNS_class_None = 254;
const uint16_t DNS_class_Any = 255;

const uint8_t DNS_opcode_Query = 0;
const uint8_t DNS_opcode_IQuery = 1;
const uint8_t DNS_opcode_Status = 2;
const uint8_t DNS_opcode_Notify = 4;
const uint8_t DNS_opcode_Update = 5;

const uint8_t DNS_type_Internet = 1;
//More types are available, see: http://www.networksorcery.com/enp/protocol/dns.htm

const uint8_t mDNS_QR_Query = 0;
const uint8_t mDNS_QR_Response = 1;

const uint8_t mDNS_RCode_No_error = 0;
const uint8_t mDNS_RCode_Format_error = 1;
const uint8_t mDNS_RCode_Server_Failure = 2;
const uint8_t mDNS_RCode_Name_Error = 3;
const uint8_t mDNS_RCode_Not_Implemented = 4;
const uint8_t mDNS_RCode_Refused = 5;

mDNS::mDNS() { }

void mDNS::begin()
{
  if (!WifiStation.isConnected())
  {
    connectionTimer.initializeMs(1000, TimerDelegate(&mDNS::begin, this)).startOnce();
    return;
  }
  //Only start server whenever the swig is conencted on a wifi

  //mDNS is multicast based, so we need to join the group
  if(!joinIgmpGroup())
  {
    return;
  }
  Debug.println("Starting mDNS server\n\r");
  if (!this->listen(PORT_NR))
  {
    Debug.println("Couldn't start mDNS server\n\r");
    return;
  }
  IPAddress currIpAddr = WifiStation.getIP();

  /*
   * /!\ WARNING /!\
   * The version of lwip that we are using with our platform uses the
   * multicast_ip field to find the output network interface while the last
   * official version is using the local_ip field...
   * If we update lwip, this will be broken!
   * /!\ WARNING /!\
   */
  udp->multicast_ip.addr = currIpAddr;

  Debug.println("mDNS server started!");
}

bool mDNS::joinIgmpGroup()
{
  IPAddress currIpAddr = WifiStation.getIP();
  host_addr.addr = currIpAddr;
  multicast_addr.addr = ANSWER_IP;

  Debug.printf("%s (%#08x) to join MC %s (%#08x)\n\r", currIpAddr.toString().c_str(), host_addr.addr, ((IPAddress)ANSWER_IP).toString().c_str(), multicast_addr.addr);

  if (espconn_igmp_join(&host_addr, &multicast_addr) != ERR_OK)
  {
    Debug.printf("ap udp_join_multigrup failed!\n");
    return false;
  }
  Debug.printf("%#08x Successfully joined ipgroup %#08x\n\r", host_addr.addr, multicast_addr.addr);
  return true;
}

struct netif* mDNS::findNetIf(IPAddress& addr)
{
  ip_addr_t ifaddr;
  ifaddr.addr = addr;
  struct netif *netif;

  /* loop through netif's */
  netif = netif_list;
  while (netif != NULL)
  {
    if (netif->ip_addr.addr == ifaddr.addr)
    {
      return netif;
    }
    /* proceed to next network interface */
    netif = netif->next;
  }

  return NULL;
}


void mDNS::onReceive(pbuf* buf, IPAddress remoteIP, uint16_t remotePort)
{
  Debug.printf("udp packet received: ip:%s port:%d\n\r", remoteIP.toString().c_str(), remotePort);
  uint16_t size = buf->tot_len;
  char* data = new char[size + 1];
  pbuf_copy_partial(buf, data, size, 0);
  data[size] = '\0';

  if (!isRequestValid(data, size, remoteIP, remotePort))
  {
    Debug.printf("not valid packet. Dump\n\r");
    return;
  }
  Debug.printf("UDP checking request\n\r");
  DNS_packet_t *mdns_packet = (DNS_packet_t*) data;
  if (!isRequestAQuery(mdns_packet, size))
  {
    Debug.printf("not valid request. Dump\n\r");
    return;
  }
  processQuery(mdns_packet, size);

  delete[] data;
}

mDNS::~mDNS()
{
  UdpConnection::close();
}

bool mDNS::isRequestValid(char* data, int size, IPAddress remoteIP, uint16_t remotePort)
{
  return remotePort == PORT_NR &&
          size > SIZEOF_DNS_PACKET_HEADER;
}

bool mDNS::isRequestAQuery(DNS_packet_t* aPacket, uint16_t size)
{
  uint16_t totalQ = ntohs(aPacket->totalQuestions);
  if(aPacket->flags.Opcode != DNS_opcode_Query)
  {
    sendNotImplemented(aPacket, size);
  }
  return aPacket->flags.Opcode == DNS_opcode_Query &&
          aPacket->flags.QR == mDNS_QR_Query &&
          totalQ > 0;
}

void mDNS::sendNotImplemented(DNS_packet_t* aPacket, uint16_t size)
{
  aPacket->flags.Rcode = mDNS_RCode_Not_Implemented;

  this->sendTo(ANSWER_IP,PORT_NR, (const char *)aPacket, size);
}

unsigned int mDNS::buildAnswerPacket(unsigned char* packetBuf, uint16_t pktId, const char *hostname)
{
  if(!hostname || strlen(hostname) == 0)
  {
    Debug.printf("buildAnswerPacket: Invalid hostname input\n\r");
    return 0;
  }
  const IPAddress MY_IP = Network.getClientIP();
  memset(packetBuf, 0, MAX_PACKET_SIZE);
  DNS_packet_t *answer = (DNS_packet_t*) packetBuf;

  answer->flags.Opcode = DNS_opcode_Query;
  answer->flags.QR = mDNS_QR_Response;
  answer->flags.AA = 1;
  answer->flags.Rcode = mDNS_RCode_No_error;

  answer->id = htons(pktId);
  answer->totalAnswerRRs = htons(1);

  char *answerName = (char *) &(answer->data);
  char *beginPos = NULL;
  unsigned char nbChar = 0;
  for(const char *curPosStr = hostname; *curPosStr != '\0'; curPosStr++)
  {
    if(*curPosStr == '.')
    {
      if(beginPos)
      {
        *beginPos = nbChar;
      }
      nbChar = 0;
      beginPos = answerName;
    }
    else
    {
      *answerName = *curPosStr;
      nbChar++;
    }
    answerName++;
  }
  if(beginPos)
  {
    *beginPos = nbChar;
  }
  else
  {
    Debug.printf("Error: hostname %s couldn't be processed correctly\n\r", hostname);
    return 0; //Malformed name
  }
  *answerName=0;

  DNS_RR_t *answerQuery = (DNS_RR_t*) (answerName + 1);
  answerQuery->type = htons(DNS_type_Internet);

  answerQuery->qclass = htons(DNS_class_Flush | DNS_class_Internet);

  answerQuery->TTL = htonl(DEFAULT_TTL);

  uint32_t currentIP = MY_IP; //Bytes are not swapped because IP@ is stored as array of uint8_t
  uint16_t rdata_length = sizeof (currentIP);
  answerQuery->rdata_length = htons(rdata_length);
  uint32_t *ptrToRData = (uint32_t *)&(answerQuery->rdata);
  memcpy(ptrToRData, &currentIP, sizeof(uint32)); //copy byte per byte to avoid LoadStoreAlignmentCause i.e. do not use *ptrToRData = currentIP

  return SIZEOF_DNS_PACKET_HEADER + SIZEOF_DNS_RR(hostname, rdata_length);
}

void mDNS::processQuery(const DNS_packet_t* aPacket, uint16_t size)
{
  AppSettings.load();
  const String & appCurrentHostName = AppSettings.hostname;
  char currentHostName[MAX_HOSTNAME_SIZE];
  m_snprintf(currentHostName, MAX_HOSTNAME_SIZE, ".%s%s", appCurrentHostName.c_str(), LOCAL_DOMAIN_NAME.c_str());
  uint16_t totalQ = ntohs(aPacket->totalQuestions);
  unsigned int currentOffset = SIZEOF_DNS_PACKET_HEADER;
  unsigned char *currentDataPtr = (unsigned char *) &(aPacket->data);
  Debug.printf("UDP nbQuery %d\n\r", totalQ);
  for (unsigned int qIdx = 0; qIdx < totalQ; qIdx++)
  {
    char currentQueryName[MAX_HOSTNAME_SIZE];
    unsigned int nbByteProcessed = readDnsName(currentQueryName, (const unsigned char *)aPacket, currentDataPtr);
    Debug.printf("UDP queryName: '%s' currentHostName: '%s'\n\r", currentQueryName, currentHostName);

    if (strncmp(currentHostName, currentQueryName, MAX_HOSTNAME_SIZE) == 0)
    {
      sendAnswer(ntohs(aPacket->id), currentHostName);
      break;
    }
    unsigned int sizeOfThisQuery = SIZEOF_DNS_QUERY(nbByteProcessed);

    currentDataPtr += sizeOfThisQuery;
    currentOffset += sizeOfThisQuery;
  }
}

unsigned int mDNS::readDnsName(char* bufferToFillin, const unsigned char *beginOfMessage, const unsigned char* ptrToData)
{
  const unsigned char *currentPtrPos = ptrToData;
  unsigned char nbCharToRead;
  unsigned int nbByteProcessed = 0;
  while((nbCharToRead = *currentPtrPos) > 0)
  {
    if(IS_MESSAGE_COMPRESSION(*currentPtrPos))
    {
      uint16_t offset;
      memcpy(&offset, currentPtrPos, sizeof(offset)); //Avoid LoadStoreAlignmentCause
      offset = ntohs(offset);
      const unsigned char *newPointerPos = beginOfMessage + GET_MESSATE_COMPRESSION_OFFSET(offset);
      readDnsName(bufferToFillin, beginOfMessage, newPointerPos);
      nbByteProcessed+=2;
      Debug.printf("\n\r1.2 return %d byte processed\n\r", nbByteProcessed);
      return nbByteProcessed;
    }
    else
    {
      *bufferToFillin = '.';
      bufferToFillin++;
      currentPtrPos++;
      nbByteProcessed++;
      for(unsigned int charCnt = 0; charCnt < nbCharToRead; charCnt++)
      {
        *bufferToFillin = *currentPtrPos;
        bufferToFillin++;
        currentPtrPos++;
        nbByteProcessed++;
      }
    }
  }
  if(nbByteProcessed == 0)
  {
    *bufferToFillin = '.';
    bufferToFillin++;
    nbByteProcessed++;
  }
  *bufferToFillin = '\0';
  nbByteProcessed++;
  Debug.printf("\n\r4 returning %d bytes processed\n\r", nbByteProcessed);
  return nbByteProcessed;
}

void mDNS::sendAnswer(uint16_t pktId, const char* currentHostName)
{
  unsigned char *packetBuf = (unsigned char *) malloc(MAX_PACKET_SIZE);

  unsigned int pktSize = buildAnswerPacket(packetBuf, pktId, currentHostName);
  if(pktSize > 0)
  {
    this->sendTo(ANSWER_IP, PORT_NR, (char *) packetBuf, pktSize);
  }

  free(packetBuf);
}
