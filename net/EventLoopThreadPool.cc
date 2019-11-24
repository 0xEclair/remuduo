#include "EventLoopThreadPool.h"

#include "EventLoop.h"
#include "EventLoopThread.h"

#include <functional>

using namespace remuduo;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop)
	:baseLoop_(baseLoop){
	
}

EventLoopThreadPool::~EventLoopThreadPool() {
	// Don't delete loop,it's stack variable
}

auto EventLoopThreadPool::start() -> void {
	assert(!started_);
	baseLoop_->assertInLoopThread();

	started_ = true;

	for(auto i=0;i<numThreads_;++i) {
		auto t = new EventLoopThread;
		threads_.push_back(t);
		loops_.emplace_back(t->startLoop());
	}
}

auto EventLoopThreadPool::getNextLoop() -> EventLoop* {
	baseLoop_->assertInLoopThread();
	auto loop = baseLoop_;
	if(!loops_.empty()) {
		// round-robin
		loop = loops_[next_];
		++next_;
		if(static_cast<size_t>(next_)>=loops_.size()) {
			next_ = 0;
		}
	}
	return loop;
}
