#pragma once

namespace remuduo {
	class Timer;
	class TimerId {
	public:
		explicit TimerId(Timer* timer)
			:value_(timer) {
			
		}

	private:

	private:
		Timer* value_;
	};
}