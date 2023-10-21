#ifndef RTSP_SERVER_H
#define RTSP_SERVER_H

#include <MThread.h>

#define RECVMAXSIZE 1024
#define SENDMAXSIZE 1024
#define RTSP_SERVER_PORT 8554

#define SERVER_RTP_PORT 55532
#define SERVER_RTCP_PORT 55533


class RtspServer : public MThread {
public:
	RtspServer(const int client_fd, const char *client_ip, const int client_port);
	virtual ~RtspServer();
	
	int initialize();

    int handleCmd_OPTIONS(char *result, int cseq);
	int handleCmd_DESCRIBE(char *reult, int cseq, char *url);
	int handleCmd_SETUP(char *result, int cseq, int clientRtpPort);
	int handleCmd_PLAY(char *result, int cseq);
	
	virtual void run() override; 
	void play(char *deviceName, int clientRtpPort, int clientRtcpPort);
	
private:

	int m_sockfd;
	char *m_ip;
	int m_port;
	
	int m_serverRtpSockfd;
	int m_serverRtcpSockfd;

	int m_clientFd;
	int m_clientPort;
	char *m_clientIp;
};

#endif
