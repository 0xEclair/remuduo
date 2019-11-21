#pragma once
#include "EventLoop.h"

#include <map>
#include <vector>

#include <muduo/base/Timestamp.h>

struct pollfd;

namespace remuduo {
	class Channel;

	class Poller :boost::noncopyable {
	public:
		Poller(EventLoop* loop);
		~Poller();

		muduo::Timestamp poll(int timeoutMs, std::vector<Channel*>* activeChannels);
		void updateChannel(Channel* channel);

		auto removeChannel(Channel* channel) -> void;

		void assertInLoopThread() { ownerLoop_->assertInLoopThread(); }
	private:
		void fillActiveChannels(int numEvents, std::vector<Channel*>* activeChannels)const;

	private:
		EventLoop* ownerLoop_;
		std::vector<pollfd> pollfds_;
		std::map<int, Channel*> channels_;
	};
}