#pragma once

#include <cstdint>

namespace remuduo {
	class Timer;
	class TimerId {
	public:
		explicit TimerId(Timer* timer=nullptr,int64_t seq=0)
			:timer_(timer),sequence_(seq) {
			
		}
		friend class TimerQueue;
	private:

	private:
		Timer* timer_;
		int64_t sequence_;
	};
}