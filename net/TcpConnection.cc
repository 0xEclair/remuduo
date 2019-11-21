#pragma once
#include "TcpConnection.h"

#include "Channel.h"
#include "EventLoop.h"
#include "Socket.h"
#include "SocketsOps.h"

#include <errno.h>
#include <stdio.h>
#include <memory>

#include <muduo/base/Logging.h>

using namespace remuduo;

TcpConnection::TcpConnection(EventLoop* loop, const std::string& nameArg, int sockfd, const InetAddress& localAddr,
	const InetAddress& peerAddr)
	:loop_(CHECK_NOTNULL(loop)),name_(nameArg),
	 socket_(new Socket(sockfd)),channel_(new Channel(loop,sockfd)),
	 localAddr_(localAddr),peerAddr_(peerAddr){
	
	LOG_DEBUG << "TcpConnection::ctor[" << name_ << "] at " << this << " fd = " << sockfd;
	channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this));
	channel_->setWriteCallback(std::bind(&TcpConnection::handleRead,this));
	channel_->setCloseCallback(std::bind(&TcpConnection::handleClose,this));
	channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
	
}

TcpConnection::~TcpConnection() {
	LOG_DEBUG << "TcpConnection::dtor[" << name_ << "] at " << this << " fd = " << channel_->fd();
}

void TcpConnection::connectEstablished() {
	loop_->assertInLoopThread();
	assert(state_ == kConnecting);
	setState(kConnected);
	channel_->enableReading();

	connectionCallback_(shared_from_this());
}

void TcpConnection::connectDestroyed() {
	loop_->assertInLoopThread();
	assert(state_ == kConnected);
	setState(kDisconnected);
	channel_->disableAll();
	connectionCallback_(shared_from_this());

	loop_->removeChannel(channel_.get());
}

auto TcpConnection::handleError() -> void {
	auto err{ sockets::getSocketError(channel_->fd()) };
	LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << muduo::strerror_tl(err);
}

auto TcpConnection::handleRead() -> void {
	char buf[65536];
	ssize_t n{ ::read(channel_->fd(), buf, sizeof buf) };
	if(n>0) {
		messageCallback_(shared_from_this(), buf, n);
	}
	else if(n==0) {
		handleClose();
	}
	else {
		handleError();
	}
}

auto TcpConnection::handleClose() -> void {
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpConnection::handleClose state = " << state_;
	assert(state_ == kConnected);
	// we don't close fd,leave it to dtor , so we can find leaks easily.
	channel_->disableAll();
	// must be the last line
	closeCallback_(shared_from_this());
}
