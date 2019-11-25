#include "TcpClient.h"

#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOps.h"

#include <stdio.h>
#include <functional>

#include <muduo/base/Logging.h>

using namespace remuduo;

namespace remuduo {
	namespace detail {
		auto removeConnection(EventLoop* loop,const TcpConnectionPtr& conn) -> void {
			loop->queueInLoop([conn=std::move(conn)]() {
				conn->connectDestroyed();
			});
		}

		auto removeConnector(const ConnectorPtr& connector) -> void {
			//connector->
		}
	}
}

TcpClient::TcpClient(EventLoop* loop, const InetAddress& serverAddr)
	:loop_(CHECK_NOTNULL(loop)), connector_{ new Connector(loop,serverAddr) }{
	connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, std::placeholders::_1));
	LOG_INFO << "TcpClient::TcpClient[" << this << "] - connector" << connector_.get();
}

TcpClient::~TcpClient() {
	LOG_INFO << "TcpClient::~TcpClient[" << this << "] - connector " << connector_.get();
	TcpConnectionPtr conn;
	{
		muduo::MutexLockGuard lock(mutex_);
		conn = connection_;
	}
	if(conn) {
		//CloseCallback cb=std::
		CloseCallback cb = std::bind(&detail::removeConnection, loop_, std::placeholders::_1);
		loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback,conn,cb));
	}
	else {
		connector_->stop();
		loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
	}
}

auto TcpClient::connect() -> void {
	LOG_INFO << "TcpClient::connect[" << this << "] - connecting to "<< connector_->serverAddress().toHostPort();
	connect_ = true;
	connector_->start();
}

auto TcpClient::disconnect() -> void {
	connect_ = false;
	{
		muduo::MutexLockGuard lock(mutex_);
		if(connection_) {
			connection_->shutdown();
		}
	}
}

auto TcpClient::stop() -> void {
	connect_ = false;
	connector_->stop();
}

auto TcpClient::retry() const -> bool {
	
}

void TcpClient::newConnection(int sockfd) {
	loop_->assertInLoopThread();
	InetAddress peerAddr = sockets::getPeerAddr(sockfd);
	char buf[32];
	snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toHostPort().data(), nextConnId_);
	++nextConnId_;
	std::string connName = buf;

	InetAddress localAddr(sockets::getLocalAddr(sockfd));
	TcpConnectionPtr conn(new TcpConnection(loop_,connName,sockfd,localAddr,peerAddr));
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, std::placeholders::_1)); // FIXME: unsafe
	{
		muduo::MutexLockGuard lock(mutex_);
		connection_ = conn;
	}
	conn->connectEstablished();
}

void TcpClient::removeConnection(const TcpConnectionPtr& conn) {
	loop_->assertInLoopThread();
	assert(loop_ == conn->getLoop());

	{
		muduo::MutexLockGuard lock(mutex_);
		assert(connection_ == conn);
		connection_.reset();
	}

	loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
	if (retry_ && connect_)
	{
		LOG_INFO << "TcpClient::connect[" << this << "] - Reconnecting to "<< connector_->serverAddress().toHostPort();
		connector_->restart();
	}
}
