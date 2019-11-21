#pragma once
#include <poll.h>
#include <functional>

#include <boost/noncopyable.hpp>
#include <muduo/base/Timestamp.h>
namespace remuduo {
	class EventLoop;

	class Channel:boost::noncopyable {
	public:
		using EventCallback = std::function<void()>;
		using ReadEventCallback = std::function<void(Timestamp)>;
		Channel(EventLoop* loop, int fd);
		~Channel();
		void handleEvent(muduo::Timestamp receiveTime);
		void setReadCallback(const ReadEventCallback& cb) { readCallback_ = cb; }
		void setWriteCallback(const EventCallback& cb) { writeCallback_ = cb; }
		void setCloseCallback(const EventCallback& cb) { closeCallback_ = cb; }
		void setErrorCallback(const EventCallback& cb) { errorCallback_ = cb; }

		int fd() const { return fd_; }
		int events() const { return events_; }
		void set_revents(int revt) { revents_ = revt; }
		bool isNoneEvent() const { return events_ == kNoneEvent; }

		void enableReading() { events_ |= kReadEvent; update(); }

		auto disableAll() -> void { events_ = kNoneEvent; update(); }
		
		// for Poller
		int index() { return index_; }
		void set_index(int idx) { index_ = idx; }

		EventLoop* ownerLoop() { return loop_; }
	private:
		void update();

		static constexpr int kNoneEvent{ 0 };
		static constexpr int kReadEvent{ POLLIN | POLLPRI };
		static constexpr int kWriteEvent{ POLLOUT };
	private:
		EventLoop* loop_;
		const int fd_;
		int events_;
		int revents_;
		int index_; // used by Poller

		bool eventHandling_ { false };
		
		ReadEventCallback readCallback_;
		EventCallback writeCallback_;
		EventCallback errorCallback_;
		EventCallback closeCallback_;
	};
}