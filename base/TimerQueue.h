#pragma once
#include "../net/Channel.h"

#include <set>
#include <memory>
#include <vector>
#include <functional>

#include <muduo/base/Mutex.h>
#include <muduo/base/Timestamp.h>
#include <boost/noncopyable.hpp>

namespace remuduo {
	class EventLoop;
	class Timer;
	class TimerId;

	class TimerQueue : boost::noncopyable {
	public:
		TimerQueue(EventLoop* loop);
		~TimerQueue();

		TimerId addTimer(const std::function<void()>& cb, muduo::Timestamp when, double interval);
		void addTimerInLoop(Timer* timer);
		//void cancel(TimerId timerId);

	private:
		void handleRead();

		using Entry = std::pair<muduo::Timestamp, Timer*>;
		std::vector<Entry> getExpired(muduo::Timestamp now);
		void reset(const std::vector<Entry>& expired, muduo::Timestamp now);

		bool insert(Timer* timer);
	private:
		EventLoop* loop_;
		const int timerfd_;
		Channel timerfdChannel_;
		// Timer list sorted by expiration
		std::set<Entry> timers_{};
	};
}
