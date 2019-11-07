#include "EventLoop.h"

#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"

#include <assert.h>
#include <muduo/base/Logging.h>


using namespace remuduo;

namespace {
	__thread EventLoop* loopInThisThread = nullptr;
	constexpr auto kPollTimeMs = 10000;
}

EventLoop::EventLoop()
	:looping_(false), quit_(false),
	 threadId_(muduo::CurrentThread::tid()),
	 poller_(new Poller(this)),
	 timerQueue_(new TimerQueue(this)){
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

TimerId EventLoop::runAt(const muduo::Timestamp& time, const std::function<void()>& cb) {
	return timerQueue_->addTimer(cb, time, 0.0);
}

TimerId EventLoop::runAfter(double delay, const std::function<void()>& cb) {
	muduo::Timestamp time(muduo::addTime(muduo::Timestamp::now(), delay));
	return runAt(time, cb);
}

TimerId EventLoop::runEvery(double interval, const std::function<void()>& cb) {
	muduo::Timestamp time(muduo::addTime(muduo::Timestamp::now(), interval));
	return timerQueue_->addTimer(cb, time, interval);
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