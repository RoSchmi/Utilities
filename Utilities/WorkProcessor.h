#pragma once

#include <vector>
#include <atomic>
#include <chrono>
#include <utility>
#include <functional>

#include "Common.h"
#include "Event.h"
#include "Optional.h"
#include "WorkQueue.h"
#include "Timer.h"

namespace util {
	template<typename T> class work_processor {
		public:
			event_single<work_processor, void, word, T&> on_item;

		private:
			work_queue<T> queue;
			std::atomic<bool> running;
			std::vector<timer<word>> workers;

			void tick(word worker) {
				try {
					T item(this->queue.dequeue());
					this->on_item(worker, item);
				}
				catch (work_queue<T>::waiter_killed_exception) {
					return;
				}
			}

		public:
			work_processor(const work_processor& other) = delete;
			work_processor& operator=(const work_processor& other) = delete;

			work_processor(word worker_count = 0, std::chrono::milliseconds delay = std::chrono::milliseconds(25)) {
				this->running = false;

				for (word i = 0; i < worker_count; i++) {
					this->workers.emplace_back(delay, i);
					auto& w = this->workers.back();
					w.on_tick += std::bind(&work_processor::tick, this, placeholders::_1);
				}
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

				this->running = false;
				this->queue = std::move(other.queue);
				this->on_item = std::move(other.on_item);
				this->workers = std::move(other.workers);

				if (was_running)
					this->start();

				return *this;
			}

			void add_work(T&& item) {
				this->queue.enqueue(std::move(item));
			}

			void start() {
				if (this->running)
					return;

				for (auto& i : this->workers)
					i.start();
			}

			void stop() {
				if (!this->running)
					return;

				for (auto& i : this->workers)
					i.stop();

				this->queue.kill_waiters();
			}
	};
}