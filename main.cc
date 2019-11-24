#include "net/Connector.h"
#include "net/EventLoop.h"

#include <stdio.h>

remuduo::EventLoop* g_loop;

void connectCallback(int sockfd)
{
	printf("connected.\n");
	g_loop->quit();
}

int main(int argc, char* argv[])
{
	remuduo::EventLoop loop;
	g_loop = &loop;
	remuduo::InetAddress addr("127.0.0.1", 9981);
	remuduo::ConnectorPtr connector(new remuduo::Connector(&loop, addr));
	connector->setNewConnectionCallback(connectCallback);
	connector->start();

	loop.loop();

	return 0;
}
