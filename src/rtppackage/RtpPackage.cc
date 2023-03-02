#include "RtspServer.h"
#include <RtpPacket.h>
#include <log.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int Rtp::startCode3(char *buf)
{
	if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1) 
		return 1;
	else
		return 0;
}

int Rtp::startCode4(char *buf)
{
	if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) 
		return 1;
	else 
		return 0;
}

char *Rtp::findNextStartCode(char *buf, int len)
{
	int i;
	if (len < 3)
		return NULL;

	for(i = 0; i < len - 3; ++i) {
		if (startCode3(buf) || startCode4(buf)) 
			return buf;
		++buf;
	}

	if (startCode3(buf))
		return buf;

	return NULL;
}

Rtp::Rtp(char *client_ip, int rtpPort, int rtcpPort) :
	clientRtpPort(rtpPort),
	clientRtcpPort(rtcpPort)
{
}

Rtp::~Rtp()
{}

int Rtp::createUdpSocket()
{
	int sockfd;
	int on = 1;

	sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sockfd < 0)
		return -1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
	return sockfd;
}

int Rtp::bindSocketAddr(int socketFd, char *ip, int port) 
{
	struct sockaddr_in addr;
	
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr((char *)ip);

	if (bind(socketFd, (sockaddr*)&addr, sizeof(sockaddr)) < 0) {
		LOG(ERROR, "failed to bind %d", port); 	
		return -1;
	}
	return 0;
}

int Rtp::rtpSendPacketUdp(RtpPacket *rtpPacket, int rtpSockfd, char *client_ip, uint32_t dataSize)
{
	struct sockaddr_in addr;
	int ret;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(clientRtpPort);
	
	addr.sin_addr.s_addr = inet_addr(client_ip);
	rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
	rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
	rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);
	
	ret = sendto(rtpSockfd,(char *)rtpPacket, (dataSize + RTP_HEADER_SIZE), 0, (sockaddr*)&addr, sizeof(addr));

	rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
	rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
	rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

	return ret;
}

int Rtp::rtpSendPacketOverTcp(int clientSockfd, RtpPacket* rtpPacket, uint32_t dataSize, char channel)
{

    rtpPacket->rtpHeader.seq = htons(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = htonl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = htonl(rtpPacket->rtpHeader.ssrc);

    uint32_t rtpSize = RTP_HEADER_SIZE + dataSize;
    char* tempBuf = (char *)malloc(4 + rtpSize);
    tempBuf[0] = 0x24;//$
    tempBuf[1] = channel;// 0x00;
    tempBuf[2] = (uint8_t)(((rtpSize) & 0xFF00) >> 8);
    tempBuf[3] = (uint8_t)((rtpSize) & 0xFF);
    memcpy(tempBuf + 4, (char*)rtpPacket, rtpSize);

    int ret = send(clientSockfd, tempBuf, 4 + rtpSize, MSG_NOSIGNAL);
	if (ret < 0)
		return ret;
   
    rtpPacket->rtpHeader.seq = ntohs(rtpPacket->rtpHeader.seq);
    rtpPacket->rtpHeader.timestamp = ntohl(rtpPacket->rtpHeader.timestamp);
    rtpPacket->rtpHeader.ssrc = ntohl(rtpPacket->rtpHeader.ssrc);

    free(tempBuf);
    tempBuf = NULL;

    return ret;
}

int Rtp::rtpSendH264Frame(RtpPacket * rtpPacket, int rtpSockfd, char *client_ip, char *frame, uint32_t frameSize)
{
	uint8_t naluType;
	int sendBytes = 0;
	int ret;
	naluType = frame[0];
	LOG(INFO, "framesize=%d \n", frameSize);

	if (frameSize < RTP_MAX_PKT_SIZE) {	
		 //*   0 1 2 3 4 5 6 7 8 9
         //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         //*  |F|NRI|  Type   | a single NAL unit ... |
         //*  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
		memcpy(rtpPacket->m_payload, frame, frameSize);
//		ret = rtpSendPacketUdp(rtpPacket ,rtpSockfd, client_ip, frameSize);
		
		ret = rtpSendPacketOverTcp(rtpSockfd, rtpPacket, frameSize, 0x00);
		if (ret < 0) 
			return -1;
		rtpPacket->rtpHeader.seq ++;
		sendBytes += ret;
		if ((naluType & 0x1F) == 7 || (naluType & 0x1F) == 8)
			goto out;
	} else {
         //*  0                   1                   2
         //*  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         //* | FU indicator  |   FU header   |   FU payload   ...  |
         //* +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+



         //*     FU Indicator
         //*    0 1 2 3 4 5 6 7
         //*   +-+-+-+-+-+-+-+-+
         //*   |F|NRI|  Type   |
         //*   +---------------+



         //*      FU Header
         //*    0 1 2 3 4 5 6 7
         //*   +-+-+-+-+-+-+-+-+
         //*   |S|E|R|  Type   |
         //*   +---------------+
		int pktNum = frameSize / RTP_MAX_PKT_SIZE;
		int remainPktSize = frameSize % RTP_MAX_PKT_SIZE;
		int i, pos = 1;

		for (i = 0; i < pktNum; i++) {
			rtpPacket->m_payload[0] = (naluType & 0x60) | 28;
			rtpPacket->m_payload[1] = naluType & 0x1F;
			if (i == 0) 
				rtpPacket->m_payload[1] |= 0x80; // start
			else if (remainPktSize == 0 && i == pktNum -1)
				rtpPacket->m_payload[1] |= 0x40; // end
			memcpy(rtpPacket->m_payload + 2, frame + pos, RTP_MAX_PKT_SIZE);
		//	ret = rtpSendPacketUdp(rtpPacket, rtpSockfd, client_ip ,RTP_MAX_PKT_SIZE + 2);
			ret = rtpSendPacketOverTcp(rtpSockfd, rtpPacket, RTP_MAX_PKT_SIZE + 2, 0x00);
			if (ret < 0)
				return -1;
			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
			pos += RTP_MAX_PKT_SIZE;
		}

		if (remainPktSize > 0) {
			rtpPacket->m_payload[0] = (naluType & 0x60) | 28;
			rtpPacket->m_payload[1] = naluType & 0x1F;
			rtpPacket->m_payload[1] |= 0x40;

			memcpy(rtpPacket->m_payload+2, frame + pos, remainPktSize + 2);
		//  ret = rtpSendPacketUdp(rtpPacket, rtpSockfd, client_ip, remainPktSize + 2);
			ret = rtpSendPacketOverTcp(rtpSockfd, rtpPacket, remainPktSize + 2, 0x00);
			if (ret < 0)
				return -1;

			rtpPacket->rtpHeader.seq++;
			sendBytes += ret;
		}
	}
	
	rtpPacket->rtpHeader.timestamp += 90000 / 30;
out:
	return sendBytes;
}
