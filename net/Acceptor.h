#include "Channel.h"
#include "Socket.h"

#include <functional>

#include <boost/noncopyable.hpp>

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
		// accept and listen
		// socket manage listen_fd with RAII
		Socket acceptSocket_;
		// channel manage listen_fd for callback to invoke cb
		Channel acceptChannel_;
		// callback for accept()
		NewConnectionCallback newConnectionCallback_;
		bool listenning_ { false };
	};
}