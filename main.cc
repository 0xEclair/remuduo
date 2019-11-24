#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include <stdio.h>

void onConnection(const remuduo::TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		printf("onConnection(): tid=%d new connection [%s] from %s\n",
			muduo::CurrentThread::tid(),
			conn->name().c_str(),
			conn->peerAddress().toHostPort().c_str());
	}
	else
	{
		printf("onConnection(): tid=%d connection [%s] is down\n",
			muduo::CurrentThread::tid(),
			conn->name().c_str());
	}
}

void onMessage(const remuduo::TcpConnectionPtr& conn,
	remuduo::Buffer* buf,
	muduo::Timestamp receiveTime)
{
	printf("onMessage(): tid=%d received %zd bytes from connection [%s] at %s\n",
		muduo::CurrentThread::tid(),
		buf->readableBytes(),
		conn->name().c_str(),
		receiveTime.toFormattedString().c_str());

	printf("onMessage(): [%s]\n", buf->retrieveAsString().c_str());
}

int main(int argc, char* argv[])
{
	printf("main(): pid = %d\n", getpid());

	remuduo::InetAddress listenAddr(9981);
	remuduo::EventLoop loop;

	remuduo::TcpServer server(&loop, listenAddr);
	server.setConnectionCallback(onConnection);
	server.setMessageCallback(onMessage);
	
	server.setThreadNum(5);
	
	server.start();

	loop.loop();
}
