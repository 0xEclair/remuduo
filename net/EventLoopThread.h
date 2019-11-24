#pragma once
#include <muduo/base/Thread.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Condition.h>
#include <boost/noncopyable.hpp>

namespace remuduo {
	class EventLoop;
	
	class EventLoopThread:boost::noncopyable{
	public:
		EventLoopThread();
		~EventLoopThread();
		EventLoop* startLoop();
		
	private:
		void threadFunc();
	private:
		EventLoop* loop_ = nullptr;
		bool exiting_ = false;
		muduo::Thread thread_;
		muduo::MutexLock mutex_;
		muduo::Condition cond_{mutex_};
	};
}

