#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include <stdio.h>

void onConnection(const remuduo::TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		printf("onConnection(): new connection [%s] from %s\n",
			conn->name().c_str(),
			conn->peerAddress().toHostPort().c_str());
	}
	else
	{
		printf("onConnection(): connection [%s] is down\n",
			conn->name().c_str());
	}
}

void onMessage(const remuduo::TcpConnectionPtr& conn,
	remuduo::Buffer* buf,
	muduo::Timestamp receiveTime)
{
	printf("onMessage(): received %zd bytes from connection [%s] at %s\n",
		buf->readableBytes(),
		conn->name().c_str(),
		receiveTime.toFormattedString().c_str());

	printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

int main()
{
	printf("main(): pid = %d\n", getpid());

	remuduo::InetAddress listenAddr(9981);
	remuduo::EventLoop loop;

	remuduo::TcpServer server(&loop, listenAddr);
	server.setConnectionCallback(onConnection);
	server.setMessageCallback(onMessage);
	server.start();

	loop.loop();

	return 0;
}
