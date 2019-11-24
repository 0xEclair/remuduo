#pragma once
#include "../base/TimerId.h"
#include "../base/TimerQueue.h"

#include <vector>

#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <muduo/base/Timestamp.h>
#include <boost/noncopyable.hpp>

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

		void runInLoop(const std::function<void()>& cb);
		void queueInLoop(const std::function<void()>& cb);

		void wakeup();
		void updateChannel(Channel* channel);

		auto removeChannel(Channel* channel) -> void;
		
		void assertInLoopThread() {
			if (!isInLoopThread()) {
				abortNotInLoopThread();
			}
		}

		bool isInLoopThread() const {
			return threadId_ == muduo::CurrentThread::tid();
		}
		static EventLoop* getEventLoopOfCurrentThread();

		void cancel(TimerId timerId);
	private:
		void abortNotInLoopThread();
		void handleRead();	// weak up
		void doPendingFunctors();
	private:

		bool looping_ = false;/* atomic */
		bool quit_ = false;
		bool callingPendingFunctors_ = false;
		const pid_t threadId_;
		muduo::Timestamp pollReturnTime_;
		
		std::unique_ptr<Poller> poller_;
		std::unique_ptr<TimerQueue> timerQueue_{ new TimerQueue(this) };
		int wakeupFd_;
		// 不像在TimerQueue里，是内部类
		// 不把 Channel 暴露给 client
		// 用于处理 wakeupFd_ 上的 readable 事件。将事件分发至handleRead()
		std::unique_ptr<Channel> wakeupChannel_{ new Channel(this,wakeupFd_) };
		std::vector<Channel*> activeChannels_;
		muduo::MutexLock mutex_;
		std::vector<std::function<void()>> pendingFunctors_;	// @BuardedBy mutex_
	};
}