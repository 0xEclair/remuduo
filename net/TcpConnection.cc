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
	channel_->setReadCallback(std::bind(&TcpConnection::handleRead,this,std::placeholders::_1));
	channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite,this));
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
	assert(state_ == kConnected || state_==kDisconnecting);
	setState(kDisconnected);
	channel_->disableAll();
	connectionCallback_(shared_from_this());

	loop_->removeChannel(channel_.get());
}

auto TcpConnection::handleError() -> void {
	int err{ sockets::getSocketError(channel_->fd()) };
	LOG_ERROR << "TcpConnection::handleError [" << name_ << "] - SO_ERROR = " << err << " " << muduo::strerror_tl(err);
}

auto TcpConnection::send(const std::string& message) -> void {
	if(state_==kConnected) {
		if(loop_->isInLoopThread()) {
			sendInLoop(message);
		}
		else {
			loop_->runInLoop([this,message=std::move(message)](){
				sendInLoop(message);
			});
		}
	}
}

auto TcpConnection::shutdown() -> void {
	// FIXME: use compare and swap
	if(state_ == kConnected) {
		setState(kDisconnecting);
		loop_->runInLoop([this]() {
			shutdownInLoop();
		});
	}
}

auto TcpConnection::setTcpNoDelay(bool on) -> void {
	socket_->setTcpNoDelay(on);
}

auto TcpConnection::sendInLoop(const std::string& message) -> void {
	loop_->assertInLoopThread();
	ssize_t nwrote{ 0 };
	// if no thing in output queue,try writing directly
	if(!channel_->isWriting() && outputBuffer_.readableBytes()==0) {
		nwrote = ::write(channel_->fd(), message.data(), message.size());
		if(nwrote>=0) {
			if(muduo::implicit_cast<size_t>(nwrote)<message.size()) {
				LOG_TRACE << "I am going to write more data";
			}
		}
		else {
			nwrote = 0;
			if (errno != EWOULDBLOCK) {
				LOG_SYSERR << "TcpConnection::sendInLoop";
			}
		}
	}

	assert(nwrote >= 0);
	if(muduo::implicit_cast<size_t>(nwrote)<message.size()) {
		outputBuffer_.append(message.data() + nwrote, message.size() - nwrote);
		if (!channel_->isWriting()) {
			channel_->enableWriting();
		}
	}
}

auto TcpConnection::shutdownInLoop() -> void {
	loop_->assertInLoopThread();
	if (!channel_->isWriting()) {
		// we are not writing
		socket_->shutdownWrite();
	}
}

auto TcpConnection::handleRead(muduo::Timestamp receiveTime) -> void {
	int savedErrno{ 0 };
	ssize_t n{inputBuffer_.readFd(channel_->fd(),&savedErrno)};
	if(n>0) {
		messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
	}
	else if(n==0) {
		handleClose();
	}
	else {
		errno = savedErrno;
		LOG_SYSERR << "TcpConnection::handleRead";
		handleError();
	}
}

auto TcpConnection::handleWrite() -> void {
	loop_->assertInLoopThread();
	if(channel_->isWriting()) {
		ssize_t n{ ::write(channel_->fd(),
						   outputBuffer_.peek(),
						   outputBuffer_.readableBytes()) };
		if(n>0) {
			outputBuffer_.retrieve(n);
			if(outputBuffer_.readableBytes()==0) {
				channel_->disableWriting();
				if(state_==kDisconnecting) {
					shutdownInLoop();
				}
			}
			else {
				LOG_TRACE << "I am going to write more data";
			}
		}
		else {
			LOG_SYSERR << "TcpConnection::handleWrite";
		}
	}
	else {
		LOG_TRACE << "Connection is down,no more writing";
	}
}

auto TcpConnection::handleClose() -> void {
	loop_->assertInLoopThread();
	LOG_TRACE << "TcpConnection::handleClose state = " << state_;
	assert(state_ == kConnected || state_ == kDisconnecting);
	// we don't close fd,leave it to dtor , so we can find leaks easily.
	channel_->disableAll();
	// must be the last line
	closeCallback_(shared_from_this());
}
