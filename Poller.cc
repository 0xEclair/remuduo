#include "Poller.h"
#include "Channel.h"

#include <poll.h>
#include <assert.h>
#include <muduo/base/Logging.h>
using namespace remuduo;

Poller::Poller(EventLoop* loop)
	:ownerLoop_(loop){
}

Poller::~Poller() {
}

muduo::Timestamp Poller::poll(int timeoutMs, std::vector<Channel*>* activeChannels) {
	auto numEvents = ::poll(&*pollfds_.begin(), pollfds_.size(), timeoutMs);
	muduo::Timestamp now(muduo::Timestamp::now());
	if(numEvents>0) {
		LOG_TRACE << numEvents << " events happended";
		fillActiveChannels(numEvents, activeChannels);
	}
	else if(numEvents==0) {
		LOG_TRACE << " nothing happended";
	}
	else {
		LOG_SYSERR << "Poller::poll()";
	}
	return now;
}
// mentance and update pollfds_
void Poller::updateChannel(Channel* channel) {
	assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
	if(channel->index()<0) {
		// a new one,add to pollfds_
		assert(channels_.find(channel->fd()) == channels_.end());
		pollfd pfd{channel->fd(),static_cast<short>(channel->events()),0};
		pollfds_.emplace_back(pfd);
		auto idx = static_cast<int>(pollfds_.size()) - 1;
		channel->set_index(idx);
		channels_[channel->fd()] = channel;
	}
	else {
		// update existing one
		assert(channels_.find(channel->fd()) != channels_.end());
		assert(channels_[channel->fd()] == channel);
		auto idx = channel->index();
		assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
		auto& pfd = pollfds_[idx];
		assert(pfd.fd == channel->fd() || pfd.fd == -1);
		pfd.events = static_cast<short>(channel->events());
		pfd.revents = 0;
		if(channel->isNoneEvent()) {
			// ignore this pollfd
			pfd.fd = -1;
		}
	}
}

void Poller::fillActiveChannels(int numEvents, std::vector<Channel*>* activeChannels) const {
	for( const auto& pfd : pollfds_ ) {
		if (numEvents <= 0)break;
		if (pfd.revents>0) {
			--numEvents;
			auto ch = channels_.find(pfd.fd);
			assert(ch != channels_.end());
			auto channel = ch->second;
			assert(channel->fd() == pfd.fd);
			channel->set_revents(pfd.revents);
			activeChannels->emplace_back(channel);
		}
	}
}