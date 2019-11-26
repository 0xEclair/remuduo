#include "EPoller.h"

#include "Channel.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include <boost/static_assert.hpp>
#include <muduo/base/Logging.h>

using namespace remuduo;

BOOST_STATIC_ASSERT(EPOLLIN==POLLIN);
BOOST_STATIC_ASSERT(EPOLLPRI==POLLPRI);
BOOST_STATIC_ASSERT(EPOLLOUT==POLLOUT);
BOOST_STATIC_ASSERT(EPOLLRDHUP==POLLRDHUP);
BOOST_STATIC_ASSERT(EPOLLERR==POLLERR);
BOOST_STATIC_ASSERT(EPOLLHUP==POLLHUP);

namespace {
	constexpr int kNew = -1;
	constexpr int kAdded = 1;
	constexpr int kDeleted = 2;
}

EPoller::EPoller(EventLoop* loop)
	:ownerLoop_(loop),
	 epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
	 events_(kInitEventListSize){
	if(epollfd_<0) {
		LOG_SYSFATAL << "EPoller::EPoller";
	}
}

EPoller::~EPoller() {
	::close(epollfd_);
}

muduo::Timestamp EPoller::poll(int timeoutMs, std::vector<Channel*>* activeChannels) {
	auto numEvents = ::epoll_wait(epollfd_, events_.data(), static_cast<int>(events_.size()), timeoutMs);
	muduo::Timestamp now(muduo::Timestamp::now());
	if (numEvents > 0) {
		LOG_TRACE << numEvents << " events happended";
		fillActiveChannels(numEvents, activeChannels);
		if (muduo::implicit_cast<size_t>(numEvents) == events_.size()) {
			events_.resize(events_.size() * 2);
		}
	}
	else if(numEvents==0) {
		LOG_TRACE << " nothing happended";
	}
	else {
		LOG_SYSERR << "EPoller::poll()";
	}
	return now;
}

auto EPoller::updateChannel(Channel* channel) -> void {
	assertInLoopThread();
	LOG_TRACE << "fd = " << channel->fd() << " events = " << channel->events();
	const auto index = channel->index();
	if( index == kNew || index == kDeleted ) {
		// a new one,add with EPOLL_CTL_ADD
		auto fd = channel->fd();
		if(index==kNew) {
			assert(channels_.find(fd) == channels_.end());
			channels_[fd] = channel;
		}
		else { // index==kDeleted
			assert(channels_.find(fd) != channels_.end());
			assert(channels_[fd]==channel);
		}
		channel->set_index(kAdded);
		update(EPOLL_CTL_ADD, channel);
	}
	else {
		// update existing one with EPOLL_CTL_MOD/DEL
		auto fd = channel->fd(); (void)fd;
		assert(channels_.find(fd) != channels_.end());
		assert(channels_[fd] == channel);
		assert(index == kAdded);
		if(channel->isNoneEvent()) {
			update(EPOLL_CTL_DEL, channel);
			channel->set_index(kDeleted);
		}
		else {
			update(EPOLL_CTL_MOD, channel);
		}
	}
}

auto EPoller::removeChannel(Channel* channel) -> void {
	assertInLoopThread();
	auto fd = channel->fd();
	LOG_TRACE << "fd = " << fd;
	assert(channels_.find(fd) != channels_.end());
	assert(channels_[fd] == channel);
	assert(channel->isNoneEvent());
	auto index = channel->index();
	assert(index == kAdded || index == kDeleted);
	auto n = channels_.erase(fd); (void)n;
	assert(n == 1);
	if (index == kAdded) {
		update(EPOLL_CTL_DEL, channel);
	}
	channel->set_index(kNew);
}

auto EPoller::fillActiveChannels(int numEvents, std::vector<Channel*>* activeChannels) const -> void {
	assert(muduo::implicit_cast<size_t>(numEvents) <= events_.size());
	for(auto i=0;i<numEvents;++i) {
		auto channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
		auto fd = channel->fd();
		auto it = channels_.find(fd);
		assert(it != channels_.end());
		assert(it->second == channel);
#endif
		channel->set_revents(events_[i].events);
		activeChannels->emplace_back(channel);
	}
}

auto EPoller::update(int operation, Channel* channel) -> void {
	epoll_event event;
	bzero(&event, sizeof event);
	event.events = channel->events();
	event.data.ptr = channel;
	auto fd = channel->fd();
	if(::epoll_ctl(epollfd_,operation,fd,&event)<0) {
		if(operation==EPOLL_CTL_DEL) {
			LOG_SYSERR << "epoll_ctl op=" << operation << " fd=" << fd;
		}
		else {
			LOG_SYSFATAL << "epoll_ctl op=" << operation << " fd=" << fd;
		}
	}
}
