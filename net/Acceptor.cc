#include "Acceptor.h"

#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

#include <muduo/base/Logging.h>

using namespace remuduo;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
	:loop_(loop),acceptSocket_(sockets::createNonBlockingOrDie()),
	 acceptChannel_(loop,acceptSocket_.fd()){

	acceptSocket_.setReuseAddr(true);
	acceptSocket_.bindAddress(listenAddr);
	acceptChannel_.setReadCallback([this](muduo::Timestamp) {
		loop_->assertInLoopThread();
		InetAddress peerAddr(0);
		int connfd{ acceptSocket_.accept(&peerAddr) };
		if(connfd>=0) {
			if(newConnectionCallback_) {
				newConnectionCallback_(std::move(connfd), peerAddr);
			}
			else {
				sockets::close(connfd);
			}
		}
	});
	
}

void Acceptor::listen() {
	loop_->assertInLoopThread();
	listenning_ = true;
	acceptSocket_.listen();
	// in enableReading() ,use update() to add to poll
	acceptChannel_.enableReading();
}