#include "EventLoop.h"

#include "Channel.h"
#include "Poller.h"

#include <assert.h>
#include <muduo/base/Logging.h>


using namespace remuduo;

namespace {
	__thread EventLoop* loopInThisThread = nullptr;
	constexpr auto kPollTimeMs = 10000;
}

EventLoop::EventLoop():looping_(false),threadId_(muduo::CurrentThread::tid()),
	quit_(false),poller_(new Poller(this)){
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
	quit_ = false;

	while(!quit_) {
		activeChannels_.clear();
		poller_->poll(kPollTimeMs, &activeChannels_);
		for(auto it:activeChannels_) {
			it->handleEvent();
		}
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit() {
	quit_ = true;
	// wakeup();
}

void EventLoop::updateChannel(Channel* channel) {
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(channel);
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