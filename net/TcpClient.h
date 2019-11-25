#pragma once

#include "TcpConnection.h"

#include <memory>

#include <muduo/base/Mutex.h>
#include <boost/noncopyable.hpp>

namespace remuduo {
	class Connector;
	using ConnectorPtr=std::shared_ptr<Connector>;
	
	class TcpClient:boost::noncopyable{
	public:
		TcpClient(EventLoop* loop, const InetAddress& serverAddr);
		~TcpClient();

		auto connect()-> void;
		auto disconnect()-> void;
		auto stop()-> void;

		auto connection() const -> TcpConnectionPtr {
			muduo::MutexLockGuard lock(mutex_);
			return connection_;
		}

		auto retry() const -> bool;
		auto enableRetry() -> void { retry_ = true; }
		
		auto setConnectionCallback(const ConnectionCallback&cb) -> void { connectionCallback_ =cb;}
		auto setMessageCallback(const MessageCallback&cb) -> void { messageCallback_ =cb;}
		auto setWriteCompleteCallback(const WriteCompleteCallback&cb) -> void { writeCompleteCallback_ =cb;}
	private:
		void newConnection(int sockfd);
		void removeConnection(const TcpConnectionPtr& conn);
	private:
		EventLoop* loop_;
		ConnectorPtr connector_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		WriteCompleteCallback writeCompleteCallback_;
		bool retry_ = false;
		bool connect_ = true;

		// always in loop thread
		int nextConnId_ = 1;
		mutable muduo::MutexLock mutex_;
		TcpConnectionPtr connection_;
	};
}

