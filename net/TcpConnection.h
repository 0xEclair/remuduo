#pragma once
#include "InetAddress.h"
#include "Buffer.h"

#include "../base/Callbacks.h"

#include <memory>
#include <string>

#include <boost/any.hpp>
#include <boost/noncopyable.hpp>

namespace remuduo {

	class Channel;
	class EventLoop;
	class Socket;

	class TcpConnection:boost::noncopyable,public std::enable_shared_from_this<TcpConnection> {
	public:
		TcpConnection(EventLoop* loop, const std::string& name, int sockfd, 
					  const InetAddress& localAddr, const InetAddress& peerAddr);
		~TcpConnection();

		void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
		void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
		void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb;  }

		void connectEstablished();
		void connectDestroyed();

		const std::string& name()const { return name_; }
		const InetAddress& localAddress() { return localAddr_; }
		const InetAddress& peerAddress() { return peerAddr_; }
		bool connected() const { return state_ == kConnected; }

		auto send(const std::string& message) -> void;
		auto shutdown() -> void;
	private:
		enum StateE{kConnecting,kConnected,kDisconnecting,kDisconnected,};

		void setState(StateE s) { state_ = s; }
		auto handleRead(muduo::Timestamp receiveTime) -> void;
		auto handleWrite() -> void;
		auto handleClose() -> void;
		auto handleError() -> void;
		auto sendInLoop(const std::string& message) -> void;
		auto shutdownInLoop() -> void;
	private:
		EventLoop* loop_;
		std::string name_;
		StateE state_ { kConnecting }; // FIXME: use atomic variable
		// we don't expose those classes to client.
		std::unique_ptr<Socket> socket_;
		std::unique_ptr<Channel> channel_;
		InetAddress localAddr_;
		InetAddress peerAddr_;
		ConnectionCallback connectionCallback_;
		MessageCallback messageCallback_;
		CloseCallback closeCallback_;
		Buffer inputBuffer_;
		Buffer outputBuffer_;
	};
}