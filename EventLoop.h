#pragma once
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>
#include <boost/noncopyable.hpp>
#include <vector>

#include "TimerId.h"

namespace remuduo {
	class Channel;
	class Poller;
	class TimerQueue;
	
	class EventLoop :boost::noncopyable{
	public:
		EventLoop();
		~EventLoop();

		void loop();

		void quit();

		TimerId runAt(const muduo::Timestamp& time, const std::function<void()>& cb);
		TimerId runAfter(double delay, const std::function<void()>& cb);
		TimerId runEvery(double interval, const std::function<void()>& cb);
		
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
		std::unique_ptr<TimerQueue> timerQueue_;
		std::vector<Channel*> activeChannels_;
	};
}