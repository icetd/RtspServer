#ifndef TCP_CONNECTION_H
#define TCP_CONNECTION_H

#define MAXCLIENT  4

#include <stdint.h>

class TcpConnection {
public:
	TcpConnection(const char *ip, int port);
	virtual ~TcpConnection();
	
	int initialize();
	int createTcpSocket();
	int bindSocketAddr();
	int acceptClient();
		
	typedef struct {
		int fd;
		int port;
		char ip[50];
	} ClientInfo_t;
	
	ClientInfo_t client_info;
    int client_index;

private:
	int m_sockfd;
	char *m_ip;
	int m_port;
};

#endif
