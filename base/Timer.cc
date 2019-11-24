#include "Timer.h"

using namespace remuduo;

muduo::AtomicInt64 Timer::s_numCreated_;

void Timer::restart(muduo::Timestamp now) {
	if(repeat_) {
		expiration_ = muduo::addTime(now, interval_);
	}
	else {
		expiration_ = muduo::Timestamp::invalid();
	}
}
