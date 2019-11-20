#include "TcpServer.h"

#include <muduo/base/Logging.h>
#include "Acceptor.h"
#include "EventLoop.h"
#include "SocketsOps.h"
#include <stdio.h>
using namespace remuduo;

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr)
	:loop_(CHECK_NOTNULL(loop)),name_(listenAddr.toHostPort()),
	 acceptor_(new Acceptor(loop,listenAddr)){
	acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));
	
}

TcpServer::~TcpServer() {
}

void TcpServer::start() {
	if(!started_) {
		started_ = true;
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
	InetAddress localAddr{ sockets::getLocalAddr(sockfd) };
	TcpConnectionPtr conn(new TcpConnection(loop_, connName, sockfd, localAddr, peerAddr));
	connections_[connName] = conn;
	conn->setConnectionCallback(connectionCallback_);
	conn->setMessageCallback(messageCallback_);
	conn->connectEstablished();
	
}
