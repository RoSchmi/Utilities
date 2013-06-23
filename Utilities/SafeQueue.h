#pragma once

#include <queue>
#include <mutex>

#include "Semaphore.h"

namespace Utilities {
	template<typename T> class SafeQueue {
		std::queue<T> baseQueue;
		std::mutex lock;
		Semaphore count;

		public:
			void enqueue(T toAdd) {
				this->lock.lock();
				this->baseQueue.push(toAdd);
				this->count.increment();
				this->lock.unlock();
			}

			bool dequeue(T& result, uint32 timeout = 0) {
				if (this->count.decrement(timeout) == Semaphore::DecrementResult::Success) {
					this->lock.lock();
					result = this->baseQueue.front();
					this->baseQueue.pop();
					this->lock.unlock();
					return true;
				}

				return false;
			}
	};
}
