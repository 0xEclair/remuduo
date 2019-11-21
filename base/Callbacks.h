#pragma once
#include <memory>
#include <functional>

#include <muduo/base/Timestamp.h>

namespace remuduo {
	class Buffer;
	class TcpConnection;
	using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
	
	using TimerCallback = std::function<void()>;
	using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
	using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer* data, Timestamp)>;
	using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
}