#pragma once

#include <thread>
#include <chrono>
#include <atomic>
#include <functional>
#include <utility>

#include "Event.h"

namespace util {
	template<typename... U> class timer {
		std::thread worker;
		std::atomic<bool> running;
		std::chrono::microseconds interval;
		std::function<void(timer*)> bound;

		void fire(U... paras) {
			this->on_tick(paras...);
		}
	
		public:
			timer(const timer& other) = delete;
			timer& operator=(const timer& other) = delete;

			event<timer, void, U...> on_tick;

			timer(std::chrono::microseconds interval, U... paras) {
				this->interval = interval;
				this->running = false;
				this->bound = std::bind(&timer::fire, placeholders::_1, paras...);
			}

			timer(timer&& other) {
				*this = std::move(other);
			}

			timer& operator=(timer&& other) {
				bool was_running = other.running.load();

				this->stop();
				other.stop();

				this->on_tick = std::move(other.on_tick);
				this->interval = other.interval;
				this->bound = std::move(other.bound);
				this->running = false;

				if (was_running)
					this->start();

				return *this;
			}
	
			void start() {
				if (this->running)
					return;
	
				this->running = true;
				this->worker = std::thread(&timer::run, this);
			}
	
			void stop() {
				if (!this->running)
					return;
	
				this->running = false;
				this->worker.join();
			}
	
			void run() {
				while (this->running) {
					std::this_thread::sleep_for(this->interval);
					this->bound(this);
				}
			}
	
	};
}