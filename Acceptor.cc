#include "Acceptor.h"

#include <muduo/base/Logging.h>
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOps.h"

using namespace remuduo;

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr)
	:loop_(loop),acceptSocket_(sockets::createNonBlockingOrDie()),
	 acceptChannel_(loop,acceptSocket_.fd()){

	acceptSocket_.setReuseAddr(true);
	acceptSocket_.bindAddress(listenAddr);
	acceptChannel_.setReadCallback([this]() {
		loop_->assertInLoopThread();
		InetAddress peerAddr(0);
		auto connfd{ acceptSocket_.accept(&peerAddr) };
		if(connfd>=0) {
			if(newConnectionCallback_) {
				newConnectionCallback_(connfd, peerAddr);
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
	acceptChannel_.enableReading();
}