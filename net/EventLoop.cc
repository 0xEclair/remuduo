#include "EventLoop.h"

#include "Poller.h"
#include "Channel.h"

#include <assert.h>
#include <signal.h>
#include <sys/eventfd.h>

#include <muduo/base/Logging.h>

using namespace remuduo;

namespace {
	__thread EventLoop* loopInThisThread = nullptr;
	constexpr auto kPollTimeMs = 10000;
	static int createEventfd() {
		auto evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
		if(evtfd<0) {
			LOG_SYSERR << "Failed in eventfd";
			abort();
		}
		return evtfd;
	}
}

class IgnoreSigPipe{
public:
	IgnoreSigPipe()
	{
		::signal(SIGPIPE, SIG_IGN);
	}
};

IgnoreSigPipe initObj;

EventLoop::EventLoop()
	:threadId_(muduo::CurrentThread::tid()),
	 poller_(new Poller(this)),
	 wakeupFd_(createEventfd()){
	
	LOG_TRACE << "EventLoop created" << this << " in thread " << threadId_;
	if(loopInThisThread) {
		LOG_FATAL << "Another EventLoop " << loopInThisThread << "exists in this thread " << threadId_;
	}
	else {
		loopInThisThread = this;
	}

	wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
	wakeupChannel_->enableReading();
}

EventLoop::~EventLoop() {
	assert(!looping_);
	::close(wakeupFd_);
	loopInThisThread = nullptr;
}

void EventLoop::loop() {
	assert(!looping_);
	assertInLoopThread();
	looping_ = true;
	quit_ = false;

	while(!quit_) {
		activeChannels_.clear();
		pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
		for(auto it:activeChannels_) {
			it->handleEvent(pollReturnTime_);
		}
		doPendingFunctors();
	}

	LOG_TRACE << "EventLoop " << this << " stop looping";
	looping_ = false;
}

void EventLoop::quit() {
	quit_ = true;
	// wakeup();
	if(!isInLoopThread()) {
		wakeup();
	}
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

void EventLoop::runInLoop(const std::function<void()>& cb) {
	if(isInLoopThread()) {
		cb();
	}
	else {
		queueInLoop(cb);
	}
}

void EventLoop::queueInLoop(const std::function<void()>& cb) {
	{
		muduo::MutexLockGuard lock(mutex_);
		pendingFunctors_.push_back(cb);
	}

	// 如果不wakeup()
	// 1.在主线程
	// 2.还未调用 doPendingFunctors()
	if( !isInLoopThread() || callingPendingFunctors_ ) {
		wakeup();
	}
}

void EventLoop::wakeup() {
	uint64_t one = 1;
	auto n = ::write(wakeupFd_, &one, sizeof one);
	if( n!=sizeof one) {
		LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
	}
}

void EventLoop::updateChannel(Channel* channel) {
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->updateChannel(channel);
}

auto EventLoop::removeChannel(Channel* channel) -> void {
	assert(channel->ownerLoop() == this);
	assertInLoopThread();
	poller_->removeChannel(channel);
}

EventLoop* EventLoop::getEventLoopOfCurrentThread() {
	return loopInThisThread;
}

void EventLoop::cancel(TimerId timerId) {
	return timerQueue_->cancel(timerId);
}

void EventLoop::abortNotInLoopThread() {
	LOG_FATAL
		<< "EventLoop::abortNotInLoopThread - EventLoop " << this
		<< "was created in threadId_ = " << threadId_
		<< ",current thread id = " << muduo::CurrentThread::tid();
}

void EventLoop::handleRead() {
	uint64_t one = 1;
	auto n = ::read(wakeupFd_, &one, sizeof one);
	if(n != sizeof one) {
		LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
	}
}

void EventLoop::doPendingFunctors() {
	std::vector<std::function<void()>> functors;
	callingPendingFunctors_ = true;
	{
		muduo::MutexLockGuard lock(mutex_);
		functors.swap(pendingFunctors_);
	}

	for(auto& func:functors) {
		func();
	}
	callingPendingFunctors_ = false;
}
