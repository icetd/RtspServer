#ifndef RTPPACKET_H
#define RTPPACKET_H

#include <stdint.h>

#define RTP_VERSION		2
#define RTP_PAYLOAD_TYPE_H264	96
#define RTP_PAYLOAD_TYPE_AAC	97

#define RTP_HEADER_SIZE		12
#define RTP_MAX_PKT_SIZE	1400

#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533


 /*
  *    0                   1                   2                   3
  *    7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0|7 6 5 4 3 2 1 0
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |                           timestamp                           |
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *   |           synchronization source (SSRC) identifier            |
  *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
  *   |            contributing source (CSRC) identifiers             |
  *   :                             ....                              :
  *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
  *
  */

typedef struct {
	uint8_t csrcLen : 4;
	uint8_t extension : 1;
	uint8_t padding : 1;
	uint8_t version : 2;

	uint8_t payloadType : 7;
	uint8_t marker : 1;
	uint16_t seq;
	uint32_t timestamp;
	uint32_t ssrc;
} RtpHeader;

typedef struct {
	RtpHeader rtpHeader;
	uint8_t m_payload[0];
} RtpPacket;

class Rtp {
public:
	Rtp(char *ip, int rtpPort, int rtcpPort);
	virtual ~Rtp();
		

	int createUdpSocket();
	int bindSocketAddr(int socketFd, char *client_ip, int port);
	
	int rtpSendH264Frame(RtpPacket *rtpPacket, int rtpSockfd, char *client_ip, char *frame, uint32_t frameSize);
	int rtpSendPacketUdp(RtpPacket *rtpPacket,  int rtpSockfd, char *client_ip, uint32_t dataSize);
	int rtpSendPacketOverTcp(int clientSockfd, RtpPacket* rtpPacket, uint32_t dataSize, char channel);

	static int startCode3(char *buf);
	static int startCode4(char *buf);
	static char *findNextStartCode(char *buf, int len);

private:

	int clientRtpPort;
	int clientRtcpPort;
};

#endif
