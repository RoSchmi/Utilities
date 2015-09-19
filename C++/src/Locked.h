#pragma once

#include <mutex>

#include "Common.h"

namespace util {
	template<typename T> class locked {
		std::mutex mtx;
		T* data;

	public:
		locked(T* item = nullptr) {
			this->data = item;
		}

		void lock() {
			this->mtx.lock();
		}

		void unlock() {
			this->mtx.unlock();
		}

		T* get() {
			return this->data;
		}

		T& operator*() {
			return *this->data;
		}

		T* operator->() {
			return this->data;
		}
	};
}