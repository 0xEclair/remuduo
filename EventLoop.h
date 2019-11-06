#pragma once
#include <muduo/base/Thread.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace remuduo {
	class Channel;
	class Poller;
	class EventLoop :boost::noncopyable{
	public:
		EventLoop();
		~EventLoop();

		void loop();

		void quit();

		void updateChannel(Channel* channel);
		
		void assertInLoopThread() {
			if (!isInLoopThread()) {
				abortNotInLoopThread();
			}
		}

		bool isInLoopThread() const {
			return threadId_ == muduo::CurrentThread::tid();
		}
		static EventLoop* getEventLoopOfCurrentThread();
	private:
		void abortNotInLoopThread();

	private:

		bool looping_;/* atomic */
		const pid_t threadId_;
		bool quit_;
		std::unique_ptr<Poller> poller_;
		std::vector<Channel*> activeChannels_;
	};
}