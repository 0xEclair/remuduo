#include "EventLoop.h"

#include <poll.h>
#include <assert.h>
#include <muduo/base/Logging.h>


using namespace remuduo;

namespace {
	__thread EventLoop* loopInThisThread = nullptr;
}

EventLoop::EventLoop():looping_(false),threadId_(muduo::CurrentThread::tid()){
	LOG_TRACE << "EventLoop created" << this << " in thread " << threadId_;
	if(loopInThisThread) {
		LOG_FATAL << "Another EventLoop " << loopInThisThread << "exists in this thread " << threadId_;
	}
	else {
		loopInThisThread = this;
	}
}

EventLoop::~EventLoop() {
	assert(!looping_);
	loopInThisThread = nullptr;
}

void EventLoop::loop() {
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;

	::poll(NULL, 0, 5 * 1000);

	LOG_TRACE << "EventLoop" << this << " stop looping";
	looping_ = false;
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
	return loopInThisThread;
}

void EventLoop::abortNotInLoopThread() {
	LOG_FATAL
		<< "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< "was created in threadId_ = " << threadId_
		<< ",current thread id = " << muduo::CurrentThread::tid();
}