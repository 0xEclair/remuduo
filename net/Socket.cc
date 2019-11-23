#include "Socket.h"

#include "InetAddress.h"
#include "SocketsOps.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h> // bzero

using namespace remuduo;

Socket::~Socket() {
	sockets::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& addr) {
	sockets::bindOrDie(sockfd_, addr.getSockAddrInet());
}

void Socket::listen() {
	sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr) {
	sockaddr_in addr;
	bzero(&addr, sizeof addr);
	int connfd{ sockets::accept(sockfd_,&addr) };
	if(connfd>=0) {
		peeraddr->setSockAddrInet(addr);
	}
	return connfd;
}

void Socket::setReuseAddr(bool on) {
	auto optval = on ? 1 : 0;
	::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

auto Socket::shutdownWrite() -> void {
	sockets::shutdownWrite(sockfd_);
}

auto Socket::setTcpNoDelay(bool on) -> void {
	int optval{ on ? 1 : 0 };
	::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
	// FIXME CHECK
}
