#include "TcpServer.h"

#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include "EventLoopThreadPool.h"

#include <stdio.h>

#include <muduo/base/Logging.h>

using namespace remuduo;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
	:loop_(CHECK_NOTNULL(loop)), name_(listenAddr.toHostPort()),
	threadPool_(new EventLoopThreadPool(loop)),
	 acceptor_(new Acceptor(loop,listenAddr)){
	
	acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
	
}

TcpServer::~TcpServer() {
}

auto TcpServer::setThreadNum(int numThreads) -> void {
	assert(numThreads >= 0);
	threadPool_->setThreadNum(numThreads);
}

auto TcpServer::removeConnection(const TcpConnectionPtr& conn) -> void {
	// FIXME: unsafe
	loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

auto TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn) -> void {
	loop_->assertInLoopThread();
	LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_ << "] - connection " << conn->name();
	auto n = connections_.erase(conn->name());
	assert(n == 1); (void)n;
	auto ioLoop = conn->getLoop();
	ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}

void TcpServer::start() {
	if(!started_) {
		started_ = true;
		threadPool_->start();
	}

	if(!acceptor_->listenning()) {
		loop_->runInLoop(std::bind(&Acceptor::listen, acceptor_.get()));
	}
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr) {
	loop_->assertInLoopThread();
	char buf[32];
	snprintf(buf, sizeof buf, "#%d", nextConnId_);
	++nextConnId_;
	std::string connName = name_ + buf;
	LOG_INFO << "TcpServer::newConnection [" << name_
		<< "] - new connection [" << connName
		<< "] from " << peerAddr.toHostPort();
	InetAddress localAddr{sockets::getLocalAddr(sockfd) };
	auto ioLoop = threadPool_->getNextLoop();
	TcpConnectionPtr conn(new TcpConnection(ioLoop, connName, sockfd, localAddr, peerAddr));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->setWriteCompleteCallback(writeCompleteCallback_);
	conn->setCloseCallback(std::bind(&TcpServer::removeConnection,this,std::placeholders::_1));
	ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}
