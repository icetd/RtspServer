#include <RtspServer.h>
#include <cstdio>
#include <cstring>
#include <iterator>
#include <strings.h>
#include <unistd.h>
#include "MThread.h"
#include "RtpPacket.h"
#include "log.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>
#include <x264encoder.h>
#include <V4l2Device.h>
#include <V4l2Capture.h>
#include <thread>


int RtspServer::handleCmd_OPTIONS(char *result, int cseq)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Public: OPTIONS, DESCRIBE, SETUP, PLAY\r\n"
			"\r\n",
			cseq);
	return 0;
}

int RtspServer::handleCmd_DESCRIBE(char *result, int cseq, char *url)
{
	char sdp[500];
	char localIp[100];

	sscanf(url, "rtsp://%[^:]:", localIp);
	sprintf(sdp, "v=0\r\n"
			"o=- 9%ld 1 IN IP4 %s\r\n"
			"t=0 0\r\n"
			"a=control:*\r\n"
			"m=video 0 RTP/AVP 96\r\n"
			"a=rtpmap:96 H264/90000\r\n"
			"a=control:track0\r\n",
			time(NULL), localIp);
	
	sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
			"Content-Base: %s\r\n"
			"Content-type: application/sdp\r\n"
			"Content-length: %zu\r\n\r\n"
			"%s",
			cseq, url, strlen(sdp), sdp);
	
	return 0;
}

int RtspServer::handleCmd_SETUP(char *result, int cseq, int clientRtpPort)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Transport: RTP/AVP/TCP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
			"\r\n",
			cseq, clientRtpPort, clientRtpPort + 1, 
			SERVER_RTP_PORT, SERVER_RTCP_PORT);

	return 0;
}

int RtspServer::handleCmd_PLAY(char *result, int cseq)
{
	sprintf(result, "RTSP/1.0 200 OK\r\n"
			"CSeq: %d\r\n"
			"Range: npt-0.000-\r\n"
			"Session: 66334873; timeout=10\r\n\r\n",
			cseq);

	return 0;
}

RtspServer::RtspServer(const int client_fd, const char *client_ip, const int client_port) :
	m_clientFd(client_fd),
	m_clientIp((char *)client_ip),
	m_clientPort(client_port)
{
}

RtspServer::~RtspServer()
{
}

int RtspServer::initialize()
{	
	return 0;
}

void RtspServer::play(char *deviceName, int clientRtpPort, int clientRtcpPort)
{
	const char *in_devname = deviceName;	
	v4l2IoType ioTypeIn  = IOTYPE_MMAP;
	int format = 0;
	int width = 640;
	int height = 480;
	int fps = 30;

	V4L2DeviceParameters param(in_devname, V4L2_PIX_FMT_YUYV, width, height, fps, ioTypeIn, DEBUG);
	V4l2Capture *videoCapture = V4l2Capture::create(param);
	X264Encoder *encoder = new X264Encoder(width, height, X264_CSP_I422);
	uint8_t *h264_buf = (uint8_t*) malloc(videoCapture->getBufferSize());

	char clientIp[50];
	strncpy(clientIp, m_clientIp, 50);	

	RtpPacket *rtpPacket = (RtpPacket *)malloc(sizeof(RtpPacket) + 50000);
	rtpPacket->rtpHeader.csrcLen = 0;
	rtpPacket->rtpHeader.extension = 0;
	rtpPacket->rtpHeader.padding = 0;
	rtpPacket->rtpHeader.version = RTP_VERSION;
	rtpPacket->rtpHeader.payloadType = RTP_PAYLOAD_TYPE_H264;
	rtpPacket->rtpHeader.marker = 0;
	rtpPacket->rtpHeader.seq = 0;
	rtpPacket->rtpHeader.timestamp = 0;
	rtpPacket->rtpHeader.ssrc = 0x88923423;

	Rtp *rtp = new Rtp(clientIp, clientRtpPort, clientRtcpPort);
	m_serverRtpSockfd = rtp->createUdpSocket();
	m_serverRtcpSockfd = rtp->createUdpSocket();

	rtp->bindSocketAddr(m_serverRtpSockfd, (char *) "0.0.0.0", SERVER_RTP_PORT);
	rtp->bindSocketAddr(m_serverRtcpSockfd, (char *) "0.0.0.0", SERVER_RTCP_PORT);

	LOG(INFO, "Start play");
	LOG(INFO, "client ip:%s:%d", m_clientIp, m_clientPort);

	if (videoCapture == nullptr) {
		LOG(WARN, "Cannot reading from V4l2 capture interface for device: %s", in_devname);
	} else {
		timeval tv;

		LOG(NOTICE, "Start reading from %s", in_devname);

		while(true) {
			tv.tv_sec = 1;
			tv.tv_usec = 0;
			int startCode = 0;
			int ret = videoCapture->isReadable(&tv);

			if(ret == 1) {
				uint8_t buffer[videoCapture->getBufferSize()];
				int resize = videoCapture->read((char*)buffer, sizeof(buffer));
				int frameSize = encoder->encode(buffer, resize, h264_buf);

				if (resize == -1) {
					LOG(NOTICE, "stop %s", strerror(errno));
				} else {
					if (rtp->startCode3((char *)h264_buf))
						startCode = 3;
					else 
						startCode = 4;
					frameSize -= startCode;
					if (frameSize > 0) {
						int re = rtp->rtpSendH264Frame(rtpPacket, m_clientFd, 
								clientIp ,(char *)(h264_buf + startCode), frameSize);
						if (re < 0) {
							free(h264_buf);
							free(rtpPacket);
							delete videoCapture;
							delete encoder;
							delete rtp;
							LOG(NOTICE, "client %s:%d offline", m_clientIp, m_clientPort);
							return;
						}
					}

					LOG(INFO, "yuv422 frame size: %d", resize);
				}
			} else if (ret == -1) {
				LOG(NOTICE, "stop %s", strerror(errno));
			}
		}
		free(h264_buf);
		free(rtpPacket);
		delete videoCapture;
		delete encoder;
		delete rtp;
	}

}

void RtspServer::run()
{
	char method[40];
	char url[100];
	char version[40];
	int CSeq;
	char devIndex;

	int clientRtpPort, clientRtcpPort;
	char *rBuf = (char *) malloc(RECVMAXSIZE);
	char *sBuf = (char *) malloc(SENDMAXSIZE);
	bzero(rBuf, (char)sizeof(rBuf));
	bzero(sBuf, (char)sizeof(sBuf));

	while (!this->isStoped()) {
		int recvLen = 0;
		LOG(NOTICE, "thread alive");
		recvLen = recv(m_clientFd, rBuf, RECVMAXSIZE, 0);
		if (recvLen < 0) {
			perror("recv");
			break;
		} 
		rBuf[recvLen] = '\0';
		std::string recvStr = rBuf;
		LOG(INFO, "C-->S\nrBuf = %s", recvStr.c_str());

		const char *sep = "\n";
		char *line = strtok(rBuf, sep);

		while(line) {
			if (strstr(line, "OPTIONS") ||
				strstr(line, "DESCRIBE") ||
				strstr(line, "SETUP") ||
				strstr(line, "PLAY")) {

				if (sscanf(line, "%s %s %s\r\n", method, url, version) != 3) {
					LOG(ERROR, "Connot parse request.");
				}
			} else if (strstr(line, "CSeq")) {
				if (sscanf(line, "CSeq: %d\r\n", &CSeq) != 1) {
					LOG(ERROR, "Connot parse request.");
				}
			} else if (!strncmp(line, "Transport:", strlen("Transport:"))) {
				if(sscanf(line, "Transport: RTP/AVP/TCP;unicast;interleaved=%d-%d\r\n",
					&clientRtpPort, &clientRtcpPort) != 2) {
					LOG(WARN, "Parse Transport RTP/AVP/TCP");
				}
			}
			line = strtok(NULL, sep);
		}
		
		if (!strcmp(method, "OPTIONS")) {
			if (handleCmd_OPTIONS(sBuf, CSeq)) {
				LOG(ERROR, "falied to handle options.");
				break;
			}
		} else if (!strcmp(method, "DESCRIBE")) {
			if (handleCmd_DESCRIBE(sBuf, CSeq, url)) {
				LOG(ERROR, "falied to handle describe.");
				break;
			}		
		} else if (!strcmp(method, "SETUP")) {
			if(handleCmd_SETUP(sBuf, CSeq, clientRtpPort)) {
				LOG(ERROR, "failed to handle setup.");
			}
		} else if (!strcmp(method, "PLAY")) {
			if (handleCmd_PLAY(sBuf, CSeq)) {
				LOG(ERROR, "failed to handle play");
				break;
			}
		} else {
			LOG(INFO, "no deifine cmd %s", method);
			break;
		}

		LOG(INFO, "S-->C\nsBuf = %s", sBuf);
		send(m_clientFd, sBuf, strlen(sBuf), 0);
		
		if (!strcmp(method, "PLAY")) {
			
			char temp;
			sscanf(url, "%*[^o]%c%c", &temp, &devIndex);
			LOG(NOTICE, "Device Index: video%c", devIndex);
			
			switch (devIndex) {
			case '0': {
				play((char *)"/dev/video0", clientRtcpPort, clientRtpPort);
				goto exit;
			}
	
			case '2': {
				play((char *)"/dev/video2", clientRtcpPort, clientRtpPort);
				goto exit;
			}

			default:
				  break;
			}
		
		}

		bzero(method, sizeof(method));
		bzero(url, sizeof(url));
		CSeq = 0;
	}
exit:
	close(m_clientFd);
	free(rBuf);
	free(sBuf);
}
