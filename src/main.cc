#include <log.h>
#include <TcpConnection.h>
#include <RtspServer.h>
#include <thread>
#include <unistd.h>


#define RTSP_PORT 8554

int LogLevel;

int main()
{	
	initLogger(NOTICE);
	
	TcpConnection *tcp = new TcpConnection("0.0.0.0", RTSP_PORT);
	LOG(NOTICE, "rtsp://127.0.0.1:%d/video<*>", RTSP_PORT);
	
	while (1)
	{
		if (tcp->acceptClient() != -1) {
			RtspServer *server = new RtspServer(tcp->client_info.fd, tcp->client_info.ip, tcp->client_info.port);
			server->start();
			server->detach();
		}
	}
	delete tcp;
	return 0;
}
