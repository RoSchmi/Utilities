#pragma once

#include <queue>
#include <mutex>

#include "Semaphore.h"

namespace Utilities {
	template<typename T> class SafeQueue {
		std::queue<T*> baseQueue;
		std::mutex lock;
		Semaphore count;

		public:
			void enqueue(T* toAdd) {
				this->lock.lock();
				this->baseQueue.push(toAdd);
				this->count.increment();
				this->lock.unlock();
			}

			T* dequeue(uint32 timeout = 0) {
				T* result = nullptr;

				if (this->count.decrement(timeout) == Semaphore::DecrementResult::Success) {
					this->lock.lock();
					result = this->baseQueue.front();
					this->baseQueue.pop();
					this->lock.unlock();
				}

				return result;
			}

			bool dequeue(T*& result, uint32 timeout = 0) {
				T* temp = this->dequeue(timeout);

				if (temp != nullptr) {
					result = temp;
					return true;
				}
				else {
					return false;
				}
			}
	};
};