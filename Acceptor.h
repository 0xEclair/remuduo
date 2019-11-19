#include <functional>
#include <boost/noncopyable.hpp>

#include "Channel.h"
#include "Socket.h"
namespace remuduo {

	class EventLoop;
	class InetAddress;
	class Acceptor:boost::noncopyable {
	public:
		using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;
		Acceptor(EventLoop* loop, const InetAddress& listenAddr);

		void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }

		bool listenning() const { return listenning_; }
		void listen();
	private:

		
	private:
		EventLoop* loop_;
		Socket acceptSocket_;
		Channel acceptChannel_;
		NewConnectionCallback newConnectionCallback_;
		bool listenning_ { false };
	};
}