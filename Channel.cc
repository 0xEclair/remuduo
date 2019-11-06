#include "Channel.h"
#include "EventLoop.h"

#include <sstream>

#include <muduo/base/Logging.h>
using namespace remuduo;


Channel::Channel(EventLoop* loop, int fd)
	:loop_(loop),fd_(fd),
	 events_(0),revents_(0),index_(-1){
	
}

void Channel::handleEvent() {
	if(revents_ & POLLNVAL) {
		LOG_WARN << "Channel::handle_event() POLLNVAL";
	}

	if(revents_ & (POLLERR|POLLNVAL)) {
		if (errorCallback_)errorCallback_();
	}
	if(revents_ & (POLLIN |POLLPRI |POLLRDHUP)) {
		if (readCallback_)readCallback_();
	}
	if(revents_ & POLLOUT) {
		if (writeCallback_)writeCallback_();
	}
}

void Channel::update() {
	loop_->updateChannel(this);
}
