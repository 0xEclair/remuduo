#pragma once
#include "TcpConnection.h"

#include "../base/Callbacks.h"

#include <map>
#include <string>
#include <memory>

#include <boost/noncopyable.hpp>

namespace remuduo {
	
	class Acceptor;
	class EventLoop;
	class EventLoopThreadPool;
	
	class TcpServer :boost::noncopyable{
	public:
		TcpServer(EventLoop* loop, const InetAddress& listenAddr);
		~TcpServer();

		auto setThreadNum(int numThreads) -> void;
		// start the server if it's not listenning.
		// It's harmless to call it multiple times.
		// Thread safe.
		void start();

		void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
		void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
		auto setWriteCompleteCallback(const WriteCompleteCallback& cb) -> void { writeCompleteCallback_ = cb; }
		
	private:
		void newConnection(int sockfd, const InetAddress& peerAddr);
		// Thread safe
		auto removeConnection(const TcpConnectionPtr& conn) -> void;
		// Not thread safe,but in loop
		auto removeConnectionInLoop(const TcpConnectionPtr& conn) -> void;
	private:
		EventLoop* loop_;
		const std::string name_;
		std::unique_ptr<Acceptor> acceptor_;
		std::unique_ptr<EventLoopThreadPool> threadPool_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		WriteCompleteCallback writeCompleteCallback_;
		bool started_ = false;
		int nextConnId_ = 1;
		std::map<std::string, TcpConnectionPtr> connections_;
	};

}