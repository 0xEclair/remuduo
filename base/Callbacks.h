#pragma once
#include <memory>
#include <functional>

namespace remuduo {
	
	class TcpConnection;
	using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
	
	using TimerCallback = std::function<void()>;
	using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
	using MessageCallback = std::function<void(const TcpConnectionPtr&, const char* data, ssize_t len)>;
}