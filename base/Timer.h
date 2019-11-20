#pragma once
#include <functional>

#include <boost/noncopyable.hpp>
#include <muduo/base/Timestamp.h>

namespace remuduo {
	class Timer:boost::noncopyable{
	public:
		Timer(const std::function<void()>& cb,muduo::Timestamp when,double interval)
			:callback_(cb),expiration_(when),interval_(interval),repeat_(interval>0.0) {
			
		}
		void run() const {
			callback_();
		}

		muduo::Timestamp expiration() const { return expiration_; }
		bool repeat() const { return repeat_; }

		void restart(muduo::Timestamp now);
	private:

	private:
		const std::function<void()> callback_;
		muduo::Timestamp expiration_;
		const double interval_;
		const bool repeat_;
	};
}