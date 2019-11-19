#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"
#include <stdio.h>

void newConnection(int sockfd, const remuduo::InetAddress& peerAddr)
{
	printf("newConnection(): accepted a new connection from %s\n",
		peerAddr.toHostPort().c_str());
	std::string t(muduo::Timestamp::now().toFormattedString());
	::write(sockfd,t.data() , t.size());
	remuduo::sockets::close(sockfd);
}

int test7()
{
	printf("main(): pid = %d\n", getpid());

	remuduo::InetAddress listenAddr(9981);
	remuduo::EventLoop loop;

	remuduo::Acceptor acceptor(&loop, listenAddr);
	acceptor.setNewConnectionCallback(newConnection);
	acceptor.listen();

	loop.loop();

	return 0;
}

auto main() ->int {
	test7();
	return 0;
}