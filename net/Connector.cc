#include "Connector.h"

#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <errno.h>

#include <muduo/base/Logging.h>

using namespace remuduo;

constexpr int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& serverAddr)
	:loop_(loop),serverAddr_(serverAddr),
	 retryDelayMs_(kInitRetryDelayMs){
	LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector() {
	LOG_DEBUG << "dtor[" << this << "]";
	
}

auto Connector::start() -> void {
	connect_ = true;
	loop_->runInLoop(std::bind(&Connector::startInLoop, this));
}

auto Connector::startInLoop() -> void {
	loop_->assertInLoopThread();
	assert(state_ == kDisconnected);
	if (connect_) {
		connect();
	}
	else {
		LOG_DEBUG << "do not connect";
	}
}

auto Connector::restart() -> void {
	loop_->assertInLoopThread();
	setState(kDisconnected);
	retryDelayMs_ = kInitRetryDelayMs;
	connect_ = true;
	startInLoop();
}

auto Connector::stop() -> void {
	connect_ = false;
	loop_->cancel(timerId_);
}



auto Connector::connect() -> void {
	auto sockfd = sockets::createNonBlockingOrDie();
	auto ret = sockets::connect(sockfd, serverAddr_.getSockAddrInet());
	auto savedErrno = (ret == 0) ? 0 : errno;
	switch (savedErrno)
	{
	case 0:
	case EINPROGRESS:
	case EINTR:
	case EISCONN:
		connecting(sockfd);
		break;

	case EAGAIN:
	case EADDRINUSE:
	case EADDRNOTAVAIL:
	case ECONNREFUSED:
	case ENETUNREACH:
		retry(sockfd);
		break;

	case EACCES:
	case EPERM:
	case EAFNOSUPPORT:
	case EALREADY:
	case EBADF:
	case EFAULT:
	case ENOTSOCK:
		LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
		sockets::close(sockfd);
		break;

	default:
		LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
		sockets::close(sockfd);
		// connectErrorCallback_();
		break;
	}
}

auto Connector::connecting(int sockfd) -> void {
	setState(kConnecting);
	assert(!channel_);
	channel_.reset(new Channel(loop_, sockfd));
	channel_->setWriteCallback([this]() {
		LOG_TRACE << "Connector::handleWrite " << state_;
		if(state_==kConnecting) {
			auto sockfd = removeAndResetChannel();
			auto err = sockets::getSocketError(sockfd);
			if(err) {
				LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << muduo::strerror_tl(err);
				retry(sockfd);
			}
			else if(sockets::isSelfConnect(sockfd)) {
				LOG_WARN << "Connector::handleWrite - Self connect";
				retry(sockfd);
			}
			else {
				setState(kConnected);
				if (connect_)
				{
					newConnectionCallback_(sockfd);
				}
				else
				{
					sockets::close(sockfd);
				}
			}
		}
		else {
			assert(state_ == kDisconnected);
		}
		});
	channel_->setErrorCallback(std::bind(&Connector::handleError, this));
	channel_->enableWriting();
}

auto Connector::handleError() -> void {
	LOG_ERROR << "Connector::handleError";
	assert(state_ == kConnecting);

	auto sockfd = removeAndResetChannel();
	auto err = sockets::getSocketError(sockfd);
	LOG_TRACE << "SO_ERROR = " << err << " " << muduo::strerror_tl(err);
	retry(sockfd);
}

auto Connector::retry(int sockfd) -> void {
	sockets::close(sockfd);
	setState(kDisconnected);
	if (connect_){
		LOG_INFO << "Connector::retry - Retry connecting to "
			<< serverAddr_.toHostPort() << " in "
			<< retryDelayMs_ << " milliseconds. ";
		timerId_ = loop_->runAfter(retryDelayMs_ / 1000.0,  // FIXME: unsafe
			std::bind(&Connector::startInLoop, this));
		retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
	}
	else{
		LOG_DEBUG << "do not connect";
	}
}

auto Connector::removeAndResetChannel() -> int {
	channel_->disableAll();
	loop_->removeChannel(channel_.get());
	auto sockfd = channel_->fd();
	// Can't reset channel_ here, because we are inside Channel::handleEvent
	loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
	return sockfd;
}

auto Connector::resetChannel() -> void {
	channel_.reset();
}
