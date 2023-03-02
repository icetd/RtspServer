#include <TcpConnection.h>
#include <log.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> 
#include <arpa/inet.h>


TcpConnection::TcpConnection(const char *ip, int port) :
	m_ip((char *)ip),
	m_port(port)
{
	initialize();
}

TcpConnection::~TcpConnection()
{
}

int TcpConnection::initialize()
{
	client_index = 0;
	createTcpSocket();
	bindSocketAddr();
	return 0;
}

int TcpConnection::createTcpSocket()
{
	int on = 1;
	m_sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_sockfd < 0) {
		LOG(ERROR, "create rtsp socket failed.");	
		return -1;
	}

	setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (const char *)&on, sizeof(on));
	return 0;
}

int TcpConnection::bindSocketAddr()
{
	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(m_port);
	addr.sin_addr.s_addr = inet_addr(m_ip);

	if (bind(m_sockfd, (sockaddr*)&addr, sizeof(sockaddr)) < 0) {
		LOG(ERROR, "bind rtsp port failed.");
		return -1;
	}

	if (listen(m_sockfd, 100) < 0)
	{
		perror("listen()");
		exit(1);
	}
	return 0;
}

int TcpConnection::acceptClient()
{
	socklen_t len = 0;
	struct sockaddr_in c_addr;

	memset(&c_addr, 0, sizeof(c_addr));
	len = sizeof(c_addr);

	LOG(NOTICE, "accept for client_index %d", client_index);
	client_info.fd = accept(m_sockfd, (struct sockaddr *)&c_addr, &len);
	if (client_info.fd == -1) {
		perror("accetp");
		return -1;
	}
	strcpy(client_info.ip, inet_ntoa(c_addr.sin_addr));
	client_info.port = ntohs(c_addr.sin_port);

	return 0;
}


