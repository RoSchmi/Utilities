#pragma once

#include <mutex>

#include "Common.h"

namespace util {
	template<typename T> class locked {
		std::mutex mtx;
		T* data;

	public:
		exported locked(T* item = nullptr) {
			this->data = item;
		}

		exported void lock() {
			this->mtx.lock();
		}

		exported void unlock() {
			this->mtx.unlock();
		}

		exported T* get() {
			return this->data;
		}

		exported T& operator*() {
			return *this->data;
		}

		exported T* operator->() {
			return this->data;
		}
	};
}