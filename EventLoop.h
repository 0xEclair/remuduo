#pragma once
#include <boost/noncopyable.hpp>
#include <muduo/base/Thread.h>

namespace remuduo {
	class EventLoop :boost::noncopyable{
	public:
		EventLoop();
		~EventLoop();

		void loop();

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

	private:
		void abortNotInLoopThread();

		bool looping_;/* atomic */
		const pid_t threadId_;
	};
}