#pragma once

#include <string>
#include <netinet/in.h>
namespace remuduo {
	class InetAddress {
	public:
		explicit InetAddress(uint16_t port);
		InetAddress(const std::string& ip, uint16_t port);
		InetAddress(const sockaddr_in& addr):addr_(addr){ }
		std::string toHostPort() const;

		const sockaddr_in& getSockAddrInet() const { return addr_; }
		void setSockAddrInet(const sockaddr_in& addr) { addr_ = addr; }
	private:

	private:
		sockaddr_in addr_;
	};
}
