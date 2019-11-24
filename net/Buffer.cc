#include "Buffer.h"

#include "SocketsOps.h"

#include <errno.h>
#include <memory>
#include <sys/uio.h>

#include <muduo/base/Logging.h>

using namespace remuduo;

auto Buffer::readFd(int fd, int* savedErrno) -> ssize_t {
	char extrabuf[65536];
	iovec vec[2];
	const size_t writable{ writableBytes() };
	vec[0].iov_base = begin() + writerIndex_;
	vec[0].iov_len = writable;
	vec[1].iov_base = extrabuf;
	vec[1].iov_len = sizeof extrabuf;
	const ssize_t n{ readv(fd,vec,2) };
	if(n<0) {
		*savedErrno = errno;
	}
	else if (muduo::implicit_cast<size_t>(n) <= writable) {
		writerIndex_ += n;
	}
	else {
		writerIndex_ = buffer_.size();
		append(extrabuf, n - writable);
	}
	return n;
}

auto Buffer::makeSpace(size_t len) -> void {
	if (writableBytes() + prependableBytes() < len + kCheapPrepend) {
		buffer_.resize(writerIndex_ + len);
	}
	else {
		assert(kCheapPrepend < readerIndex_);
		auto readable = readableBytes();
		std::copy(begin() + readerIndex_, begin() + writerIndex_, begin() + kCheapPrepend);
		readerIndex_ = kCheapPrepend;
		writerIndex_ = readerIndex_ + readable;
		assert(readable == readableBytes());
	}
}
