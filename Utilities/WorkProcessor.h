#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <utility>
#include <functional>

#include "Common.h"
#include "Event.h"
#include "Optional.h"
#include "WorkQueue.h"

namespace util {
	template<typename T> class work_processor {
		public:
			event_single<work_processor, void, word, T&> on_item;

		private:
			work_queue<T> queue;
			std::atomic<bool> running;
			std::vector<std::thread> workers;
			std::chrono::milliseconds delay;
			word worker_count;

			void run(word worker) {
				try {
					while (this->running) {
						std::this_thread::sleep_for(this->delay);

						T item(this->queue.dequeue());
						this->on_item(worker, item);
					}
				}
				catch (work_queue<T>::waiter_killed_exception) {
					return;
				}
			}

		public:
			work_processor(const work_processor& other) = delete;
			work_processor& operator=(const work_processor& other) = delete;

			work_processor(word worker_count = 0, std::chrono::milliseconds delay = std::chrono::milliseconds(25)) {
				this->delay = delay;
				this->worker_count = worker_count;
			}

			work_processor(work_processor&& other) {
				this->running = false;

				*this = std::move(other);
			}

			~work_processor() {
				this->stop();
			}

			work_processor& operator=(work_processor&& other) {
				bool was_running = other.running.load();

				other.stop();
				this->stop();

				this->delay = other.delay;
				this->worker_count = other.worker_count;
				this->running = false;
				this->queue = std::move(other.queue);
				this->on_item = std::move(other.on_item);

				this->on_item.event_added = std::bind(&work_processor::start, this);
				this->on_item.event_removed = std::bind(&work_processor::stop, this);
				other.on_item.event_added = nullptr;
				other.on_item.event_removed = nullptr;

				if (was_running)
					this->start();

				return *this;
			}

			void add_work(T&& item) {
				this->queue.enqueue(std::move(item));
			}

			void start() {
				this->running = true;

				for (word i = 0; i < this->worker_count; i++)
					this->workers.emplace_back(&work_processor::run, this, i);
			}

			void stop() {
				if (!this->running)
					return;

				this->running = false;
				this->queue.kill_waiters();
				for (auto& i : this->workers)
					i.join();
			}
	};
}