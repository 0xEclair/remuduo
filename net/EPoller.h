#pragma once
#include "EventLoop.h"

#include <map>
#include <vector>

#include <muduo/base/Timestamp.h>

struct epoll_event;

namespace remuduo {
	class Channel;
	
	class EPoller :boost::noncopyable{
	public:
		EPoller(EventLoop* loop);
		~EPoller();

		muduo::Timestamp poll(int timeoutMs, std::vector<Channel*>* activeChannels);

		auto updateChannel(Channel* channel) -> void;
		auto removeChannel(Channel* channel) -> void;
		auto assertInLoopThread() -> void { ownerLoop_->assertInLoopThread(); }
	private:
		static constexpr int kInitEventListSize = 16;
		auto fillActiveChannels(int numEvents,std::vector<Channel*>* activeChannels) const -> void;
		auto update(int operation,Channel* channel) -> void;
	private:
		EventLoop* ownerLoop_;
		int epollfd_;
		std::vector<epoll_event> events_;
		std::map<int, Channel*> channels_;
	};
}