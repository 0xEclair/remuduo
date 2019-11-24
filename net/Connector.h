#pragma once

#include "InetAddress.h"

#include "../base/TimerId.h"

#include <functional>
#include <memory>
#include <boost/noncopyable.hpp>
namespace remuduo {

	class Channel;
	class EventLoop;

	class Connector:boost::noncopyable {

	public:
		using NewConnectionCallback = std::function<void(int sockfd)>;

		Connector(EventLoop* loop, const InetAddress& serverAddr);
		~Connector();

		auto setNewConnectionCallback(const NewConnectionCallback& cb) -> void {
			newConnectionCallback_ = cb;
		}

		auto start() -> void;
		auto restart() -> void;
		auto stop() -> void;

		const InetAddress& serverAddress() const { return serverAddr_; }
	private:
		enum States{kDisconnected,kConnecting,kConnected};
		static constexpr int kMaxRetryDelayMs = 30 * 1000;
		static constexpr int kInitRetryDelayMs = 500;

		auto setState(States s) -> void { state_ = s; }
		auto startInLoop() -> void;
		auto connect() -> void;
		auto connecting(int sockfd) -> void;
		//auto handleWrite() -> void;
		auto handleError() -> void;
		auto retry(int sockfd) -> void;
		auto removeAndResetChannel() -> int;
		auto resetChannel() -> void;
	private:
		EventLoop* loop_;
		InetAddress serverAddr_;
		bool connect_ = false;
		States state_ = kDisconnected;
		std::unique_ptr<Channel> channel_;
		NewConnectionCallback newConnectionCallback_;
		int retryDelayMs_;
		TimerId timerId_;
	};
	using ConnectorPtr = std::unique_ptr<Connector>;
}