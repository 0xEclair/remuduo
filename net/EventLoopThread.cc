#include "EventLoop.h"

#include "EventLoopThread.h"

using namespace remuduo;

EventLoopThread::EventLoopThread()
	:thread_(std::bind(&EventLoopThread::threadFunc,this)){
	
}

EventLoopThread::~EventLoopThread() {
	exiting_ = true;
	loop_->quit();
	thread_.join();
}

EventLoop* EventLoopThread::startLoop() {
	assert(!thread_.started());
	thread_.start();
	{
		muduo::MutexLockGuard lock(mutex_);
		while(loop_==nullptr) {
			cond_.wait();
		}
	}

	return loop_;
}

void EventLoopThread::threadFunc() {
	EventLoop loop;
	{
		muduo::MutexLockGuard lock(mutex_);
		loop_ = &loop;
		cond_.notify();
	}

	loop.loop();
}
