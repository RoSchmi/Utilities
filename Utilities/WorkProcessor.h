#pragma once

#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

#include "Common.h"
#include "Event.h"
#include "WorkQueue.h"

namespace util {
	template<typename T> class work_processor {
	public:
		event_single<work_processor<T>, void, word, T&> on_item;

	private:
		work_queue<T> queue;
		std::atomic<bool> running;
		std::vector<std::thread> workers;
		std::chrono::milliseconds delay;
		word worker_count;

		void run(word worker) {
			T item;
			while (this->running) {
				std::this_thread::sleep_for(this->delay);
				if (!this->queue.dequeue(item, std::chrono::seconds(5)))
					continue;

				this->on_item(worker, item);
			}
		}

	public:
		work_processor(const work_processor& other) = delete;
		work_processor(work_processor&& other) = delete;

		work_processor& operator=(const work_processor& other) = delete;
		work_processor& operator=(work_processor&& other) = delete;

		work_processor(word workers, std::chrono::milliseconds delay = std::chrono::milliseconds(25)) {
			this->delay = delay;
			this->workers = workers;

			this->on_item.event_added = []() { this->start(); };
			this->on_item.event_removed = []() { this->stop(); };
		}

		~work_processor() {
			this->stop();
		}

		void add_work(T item) {
			this->queue.enqueue(std::move(item));
		}

		void start() {
			this->running = true;

			for (word i = 0; i < this->workers; i++)
				this->workers.emplace_back(work_processor<T>::run, this, i);
		}

		void stop() {
			if (!this->running)
				return;

			this->running = false;
			for (auto i : this->workers)
				i.join();
		}
	};
}