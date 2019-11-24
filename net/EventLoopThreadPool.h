#pragma once

#include <vector>

#include <muduo/base/Condition.h>
#include <muduo/base/Mutex.h>
#include <muduo/base/Thread.h>
#include <boost/noncopyable.hpp>
#include <boost/ptr_container/ptr_vector.hpp>

namespace remuduo {
	class EventLoop;
	class EventLoopThread;
	
	class EventLoopThreadPool : boost::noncopyable {
	public:
		EventLoopThreadPool(EventLoop* baseLoop);
		~EventLoopThreadPool();
		auto setThreadNum(int numThreads) -> void { numThreads_ = numThreads; }
		auto start() -> void;
		auto getNextLoop() -> EventLoop*;
	private:

	private:
		EventLoop* baseLoop_;
		
		bool started_ = false;
		int numThreads_ = 0;
		int next_ = 0;	// always in loop thread
		
		// 类似于std::vector<std::unique_ptr<EventLoopThread>>
		boost::ptr_vector<EventLoopThread> threads_;
		std::vector<EventLoop*> loops_;
	};
}