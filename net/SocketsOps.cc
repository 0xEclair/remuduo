#include "SocketsOps.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>	// snprintf
#include <strings.h> // bzero
#include <sys/socket.h>
#include <unistd.h>

#include <muduo/base/Logging.h>

using namespace remuduo;
namespace {
	using SA = sockaddr;

	const SA* sockaddr_cast(const sockaddr_in* addr) {
		return static_cast<const SA*>(muduo::implicit_cast<const void*>(addr));
	}

	SA* sockaddr_cast(sockaddr_in* addr) {
		return static_cast<SA*>(muduo::implicit_cast<void*>(addr));
	}

	void setNonBlockAndCloseOnExec(int sockfd) {
		auto flags = ::fcntl(sockfd,F_GETFL,0) ;
		flags |= O_NONBLOCK;
		auto ret = ::fcntl(sockfd,F_SETFL,flags);
		// FIXME check

		// close-on-exec
		flags = ::fcntl(sockfd, F_GETFD, 0);
		flags |= FD_CLOEXEC;
		ret = ::fcntl(sockfd, F_SETFD, flags);
		// FIXME check
	}
}

int sockets::createNonBlockingOrDie() {
#if VALGRIND
	int sockfd = ::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
	if(sockfd<0) {
		LOG_SYSFATAL << "sockets::createNonblockingOrDie";
	}
	setNonBlockAndCloseOnExec(sockfd);
#else
	auto sockfd = int(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP));
	if(sockfd<0) {
		LOG_SYSFATAL << "sockets::createNonblockingOrDie";
	}
#endif
	return sockfd;
}

void sockets::bindOrDie(int sockfd, const sockaddr_in& addr) {
	auto ret = int(::bind(sockfd, sockaddr_cast(&addr), sizeof addr));
	if(ret<0) {
		LOG_SYSFATAL << "sockets::bindOrDie";
	}
}

void sockets::listenOrDie(int sockfd) {
	auto ret = ::listen(sockfd,SOMAXCONN);
	if(ret<0) {
		LOG_SYSFATAL << "sockets::listenOrDie";
	}
}

int sockets::accept(int sockfd, sockaddr_in* addr) {
	socklen_t addrlen = sizeof *addr;
#if VALGRIND
	auto connfd = ::accept(sockfd,sockaddr_cast(addr),&addrlen);
	setNonBlockAndCloseOnExec(connfd);
#else
	auto connfd = ::accept4(sockfd,sockaddr_cast(addr),&addrlen,SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
	if(connfd<0) {
		auto savedErrno = errno;
		LOG_SYSERR << "Socket::accept";
		switch(savedErrno) {
		case EAGAIN:
		case ECONNABORTED:
		case EINTR:
		case EPROTO:
		case EPERM:
		case EMFILE:
			errno = savedErrno;
			break;
		case EBADF:
		case EFAULT:
		case EINVAL:
		case ENFILE:
		case ENOBUFS:
		case ENOMEM:
		case ENOTSOCK:
		case EOPNOTSUPP:
			LOG_FATAL << "unexpected error of ::accept " << savedErrno;
			break;
		default:
			LOG_FATAL << "unknown error of ::accept " << savedErrno;
			break;
		}
	}
	return connfd;
}

void sockets::close(int sockfd) {
	if(::close(sockfd)<0) {
		LOG_SYSERR << "sockets::close";
	}
}

auto sockets::shutdownWrite(int sockfd) -> void {
	if(::shutdown(sockfd,SHUT_WR)<0) {
		LOG_SYSERR << "sockets::shutdownWrite";
	}
}

void sockets::toHostPort(char* buf, size_t size, const sockaddr_in& addr){
	char host[INET_ADDRSTRLEN] = "INVALID";
	::inet_ntop(AF_INET, &addr.sin_addr, host, sizeof host);
	auto port = sockets::networkToHost16(addr.sin_port);
	snprintf(buf, size, "%s:%u", host, port);
}

void sockets::fromHostPort(const char* ip, uint64_t port, sockaddr_in* addr) {
	addr->sin_family = AF_INET;
	addr->sin_port = hostToNetwork16(port);
	if(::inet_pton(AF_INET,ip,&addr->sin_addr)<=0) {
		LOG_SYSERR << "sockets::fromHostPort";
	}
}

sockaddr_in sockets::getLocalAddr(int sockfd) {
	sockaddr_in localaddr;
	bzero(&localaddr, sizeof localaddr);
	socklen_t addrlen{ sizeof(localaddr) };
	if(::getsockname(sockfd,sockaddr_cast(&localaddr),&addrlen)<0) {
		LOG_SYSERR << "sockets::getLocalAddr ";
	}
	return localaddr;
}

sockaddr_in sockets::getPeerAddr(int sockfd) {
	sockaddr_in peeraddr;
	bzero(&peeraddr, sizeof peeraddr);
	socklen_t addrlen = sizeof(peeraddr);
	if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0)
	{
		LOG_SYSERR << "sockets::getPeerAddr";
	}
	return peeraddr;
}

auto sockets::getSocketError(int sockfd) -> int {
	int optval;
	socklen_t optlen = sizeof optval;
	if(::getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&optval,&optlen)<0) {
		return errno;
	}
	else {
		return optval;
	}
}

auto sockets::connect(int sockfd, const sockaddr_in& addr) -> int {
	return ::connect(sockfd, sockaddr_cast(&addr), sizeof addr);
}

auto sockets::isSelfConnect(int sockfd) -> bool {
	auto localaddr = getLocalAddr(sockfd);
	auto peeraddr = getPeerAddr(sockfd);
	return localaddr.sin_port == peeraddr.sin_port
		&& localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}
