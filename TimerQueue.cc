#include "TimerQueue.h"

#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"

#include <stdint.h>
#define __UINTPTR_MAX__
#include <sys/timerfd.h>
#include <muduo/base/Logging.h>

using namespace remuduo;

namespace remuduo{
	namespace details {

		int createTimerfd() {
			// FD_CLOEXEC 使用exec时，此fd被关闭，不能再使用，但是fork的子进程里面，未关闭，仍可调用
			// 怀疑是close(tfd)而已
			auto timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
			if(timerfd<0) {
				LOG_SYSFATAL << "Failed in timerfd_create";
			}
			return timerfd;
		}
		
		timespec howMuchTimeFromNow(muduo::Timestamp when) {
			auto microseconds = when.microSecondsSinceEpoch() - muduo::Timestamp::now().microSecondsSinceEpoch();
			if(microseconds<100) {
				microseconds = 100;
			}
			timespec ts;
			ts.tv_sec = static_cast<time_t>(microseconds / muduo::Timestamp::kMicroSecondsPerSecond);
			ts.tv_nsec = static_cast<long>((
				microseconds % muduo::Timestamp::kMicroSecondsPerSecond) * 1000);
			return ts;
		}

		void resetTimerfd(int timerfd,muduo::Timestamp expiration) {
			itimerspec newValue, oldValue;
			bzero(&newValue, sizeof(newValue));
			bzero(&oldValue, sizeof(oldValue));
			newValue.it_value = howMuchTimeFromNow(expiration);
			auto ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
			if(ret) {
				LOG_SYSERR << "timerfd_settime() ";
			}
		}

		void readTimerfd(int timerfd,muduo::Timestamp now) {
			uint64_t howmany;
			auto n = ::read(timerfd, &howmany, sizeof(howmany));
			LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
			if(n!=sizeof(howmany)) {
				LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
			}
		}
	}
}

using namespace remuduo::details;

TimerQueue::TimerQueue(EventLoop* loop)
	:loop_(loop),timerfd_(createTimerfd()),
	 timerfdChannel_(loop,timerfd_){
	timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));
	timerfdChannel_.enableReading();
}

TimerQueue::~TimerQueue() {
	::close(timerfd_);
	for(auto it:timers_) {
		delete it.second;
	}
}

TimerId TimerQueue::addTimer(const std::function<void()>& cb, muduo::Timestamp when, double interval) {
	auto timer = new Timer(cb, when, interval);
	loop_->assertInLoopThread();
	auto earliestChanged = insert(timer);
	if(earliestChanged) {
		resetTimerfd(timerfd_, timer->expiration());
	}
	return TimerId(timer);
	
}

void TimerQueue::handleRead() {
	loop_->assertInLoopThread();
	muduo::Timestamp now(muduo::Timestamp::now());
	readTimerfd(timerfd_, now);

	std::vector<Entry> expired = getExpired(now);

	for(auto it:expired) {
		it.second->run();
	}

	reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::getExpired(muduo::Timestamp now) {
	std::vector<Entry> expired;
	// 生成现在的时间戳，且Timer* 为最大值
	// 比UINTPTR_MAX小的都在内
	// sentry就是key
	Entry sentry = std::make_pair(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
	auto it = timers_.lower_bound(sentry);
	assert(it == timers_.end() || now < it->first);
	std::copy(timers_.begin(), it, back_inserter(expired));
	timers_.erase(timers_.begin(), it);

	return expired;
}

void TimerQueue::reset(const std::vector<Entry>& expired, muduo::Timestamp now) {
	muduo::Timestamp nextExpire;
	for(auto it:expired) {
		if(it.second->repeat()) {
			it.second->restart(now);
			insert(it.second);
		}
		else {
			delete it.second;
		}
	}

	if(!timers_.empty()) {
		nextExpire = timers_.begin()->second->expiration();
	}

	if(nextExpire.valid()) {
		resetTimerfd(timerfd_, nextExpire);
	}
}
 
bool TimerQueue::insert(Timer* timer) {
	auto earliestChanged=false;
	auto when = timer->expiration();
	auto it = timers_.begin();
	if(it==timers_.end()||when<it->first) {
		earliestChanged = true;
	}
	auto result = timers_.insert(std::make_pair(when, timer));
	assert(result.second);
	return earliestChanged;
}
