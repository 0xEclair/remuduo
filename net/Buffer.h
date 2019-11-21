#pragma once
#include "../base/copyable.h"

#include <algorithm>
#include <string>
#include <vector>

#include <assert.h>

namespace remuduo {

	class Buffer:public copyable {
	public:
		static constexpr int kCheapPrepend{ 8 };
		static constexpr int kInitialSize{ 1024 };

		Buffer()
			:buffer_(kCheapPrepend+kInitialSize),
			 readerIndex_(kCheapPrepend),
			 writerIndex_(kCheapPrepend) {

			
		}

		auto swap(Buffer& rhs) -> void {
			buffer_.swap(rhs.buffer_);
			std::swap(readerIndex_, rhs.readerIndex_);
			std::swap(writerIndex_, rhs.writerIndex_);
		}

		auto readableBytes() const -> size_t {
			return writerIndex_ - readerIndex_;
		}

		auto writableBytes() const -> size_t {
			return buffer_.size() - writerIndex_;
		}

		auto prependableBytes() const -> size_t {
			return readerIndex_;
		}

		const char* peek() const {
			return begin() + readerIndex_;
		}

		auto retrieve(size_t len) -> void {
			assert(len <= readableBytes());
			readerIndex_ += len;
		}

		auto retrieveUntil(const char* end) -> void {
			assert(peek() <= end);
			assert(end <= beginWrite());
			retrieve(end - peek());
		}

		auto retrieveAll() {
			readerIndex_ = kCheapPrepend;
			writerIndex_ = kCheapPrepend;
		}
		
		auto retrieveAsString() -> std::string {
			std::string str(peek(), readableBytes());
			retrieveAll();
			return str;
		}

		auto append(const std::string& std) -> void {
			//append(std.data(), str.length());
		}

		auto append(const char* data,size_t len) -> void {

			
		}
	private:
		char* begin() { return &*buffer_.begin(); }
		const char* begin() const { return &*buffer_.begin(); }
		auto makeSpace(size_t len) -> void {
			
		}
	private:
		std::vector<char> buffer_;
		size_t readerIndex_;
		size_t writerIndex_;
	};
}