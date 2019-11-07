#include "Timer.h"

using namespace remuduo;

void Timer::restart(muduo::Timestamp now) {
	if(repeat_) {
		expiration_ = muduo::addTime(now, interval_);
	}
	else {
		expiration_ = muduo::Timestamp::invalid();
	}
}
