#include "net/TcpServer.h"
#include "net/EventLoop.h"
#include "net/InetAddress.h"
#include <stdio.h>

std::string message;

void onConnection(const remuduo::TcpConnectionPtr& conn)
{
	if (conn->connected())
	{
		printf("onConnection(): tid=%d new connection [%s] from %s\n",
			muduo::CurrentThread::tid(),
			conn->name().c_str(),
			conn->peerAddress().toHostPort().c_str());
		conn->send(message);
	}
	else
	{
		printf("onConnection(): tid=%d connection [%s] is down\n",
			muduo::CurrentThread::tid(),
			conn->name().c_str());
	}
}

void onWriteComplete(const remuduo::TcpConnectionPtr& conn)
{
	conn->send(message);
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

	buf->retrieveAll();
}

int main(int argc, char* argv[])
{
	printf("main(): pid = %d\n", getpid());

	std::string line;
	for (int i = 33; i < 127; ++i)
	{
		line.push_back(char(i));
	}
	line += line;

	for (size_t i = 0; i < 127 - 33; ++i)
	{
		message += line.substr(i, 72) + '\n';
	}

	remuduo::InetAddress listenAddr(9982);
	remuduo::EventLoop loop;

	remuduo::TcpServer server(&loop, listenAddr);
	server.setConnectionCallback(onConnection);
	server.setMessageCallback(onMessage);
	server.setWriteCompleteCallback(onWriteComplete);

	server.setThreadNum(5);

	server.start();

	loop.loop();
}
