#pragma once

#include <arpa/inet.h>
#include <endian.h>

namespace remuduo {
namespace sockets {

	inline uint64_t hostToNetwork64(uint64_t host64){
		return htobe64(host64);
	}

	inline uint32_t hostToNetwork32(uint32_t host32){
		return htonl(host32);
	}

	inline uint16_t hostToNetwork16(uint16_t host16){
		return htons(host16);
	}

	inline uint64_t networkToHost64(uint64_t net64){
		return be64toh(net64);
	}

	inline uint32_t networkToHost32(uint32_t net32){
		return ntohl(net32);
	}

	inline uint16_t networkToHost16(uint16_t net16){
		return ntohs(net16);
	}

	int createNonBlockingOrDie();

	void bindOrDie(int sockfd, const sockaddr_in& addr);
	void listenOrDie(int sockfd);
	int accept(int sockfd, sockaddr_in* addr);
	void close(int sockfd);
	auto shutdownWrite(int sockfd) -> void;
	
	void toHostPort(char* buf, size_t size, const sockaddr_in& addr);
	void fromHostPort(const char* ip, uint64_t port, sockaddr_in* addr);

	sockaddr_in getLocalAddr(int sockfd);
	sockaddr_in getPeerAddr(int sockfd);

	auto getSocketError(int sockfd) -> int;

	auto connect(int sockfd, const sockaddr_in& addr) -> int;

	auto isSelfConnect(int sockfd) -> bool;
}
}