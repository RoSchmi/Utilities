#pragma once

#include <chrono>
#include <utility>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <type_traits>

namespace util {
	template<typename T> class work_queue {
		static_assert(std::is_move_assignable<T>::value && std::is_move_constructible<T>::value, "work_queue type T must be move assignable and constructible.");

		std::queue<T> items;
		std::mutex lock;
		std::condition_variable cv;

		public:
			work_queue(const work_queue& other) = delete;
			work_queue(work_queue&& other) = delete;

			work_queue& operator=(const work_queue& other) = delete;
			work_queue& operator=(work_queue&& other) = delete;

			work_queue() = default;
			~work_queue() = default;

			void enqueue(T&& item) {
				std::unique_lock<std::mutex> lock(this->lock);
				this->items.push(std::move(item));
				this->cv.notify_one();
			}

			bool dequeue(T& target, std::chrono::seconds timeout) {
				std::unique_lock<std::mutex> lock(this->lock);

				if (this->items.empty())
					if (this->cv.wait_for(lock, timeout) == std::cv_status::timeout)
						return false;

				target = std::move(this->items.front());
				this->items.pop();

				return true;
			}

			void dequeue(T& target) {
				std::unique_lock<std::mutex> lock(this->lock);

				while (this->items.empty())
					this->cv.wait(lock);

				target = std::move(this->items.front());
				this->items.pop();
			}

			T dequeue() {
				std::unique_lock<std::mutex> lock(this->lock);

				while (this->items.empty())
					this->cv.wait(lock);

				T request(std::move(this->items.front()));
				this->items.pop();

				return std::move(request);
			}
	};
}