#pragma once
#include <map>
#include <string>
#include <memory>
#include <boost/noncopyable.hpp>

#include "Callbacks.h"
#include "TcpConnection.h"
namespace remuduo {
	
	class Acceptor;
	class EventLoop;
	
	class TcpServer :boost::noncopyable{
	public:
		TcpServer(EventLoop* loop, const InetAddress& listenAddr);
		~TcpServer();

		// start the server if it's not listenning.
		// It's harmless to call it multiple times.
		// Thread safe.
		void start();

		void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
		void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
		
	private:
		void newConnection(int sockfd, const InetAddress& peerAddr);
	private:
		EventLoop* loop_;
		const std::string name_;
		std::unique_ptr<Acceptor> acceptor_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		bool started_{false};
		int nextConnId_{1};
		std::map<std::string, TcpConnectionPtr> connections_;
	};

}