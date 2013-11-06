#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>
#include <functional>

#include "Common.h"

namespace util {
	template<typename... U> class event {
	public:
		typedef std::function<void(U...)> handler_type;

		event() = default;
		event(const event& other) = delete;
		event& operator=(const event& other) = delete;

	private:
		std::vector<handler_type> handlers;
		std::mutex lock;

	public:
		std::function<void()> event_added;
		std::function<void()> event_removed;

		exported event(event&& other) {
			*this = std::move(other);
		}

		exported event& operator=(event&& other) {
			std::unique_lock<std::mutex> lck1(this->lock);
			std::unique_lock<std::mutex> lck2(other.lock);

			this->handlers = std::move(other.handlers);

			return *this;
		}

		exported void operator+=(handler_type hndlr) {
			std::unique_lock<std::mutex> lck(this->lock);

			this->handlers.push_back(hndlr);

			if (this->event_added)
				this->event_added();
		}

		template<typename... V> exported void operator()(V&&... paras) {
			std::unique_lock<std::mutex> lck(this->lock);

			for (auto i : this->handlers)
				i(std::forward<V>(paras)...);
		}
	};

	template<typename T, typename... U> class event_single {
	public:
		typedef std::function<T(U...)> handler_type;

		class event_already_set {};

		event_single() = default;
		event_single(const event_single& other) = delete;
		event_single& operator=(const event_single& other) = delete;

	private:
		handler_type handler;
		std::mutex lock;

	public:
		std::function<void()> event_added;
		std::function<void()> event_removed;

		exported event_single(event_single&& other) {
			*this = std::move(other);
		}

		exported event_single& operator=(event_single&& other) {
			std::unique_lock<std::mutex> lck1(this->lock);
			std::unique_lock<std::mutex> lck2(other.lock);

			this->handler = other.handler;
			other.handler = nullptr;

			return *this;
		}

		exported void operator+=(handler_type func) {
			std::unique_lock<std::mutex> lck(this->lock);

			if (this->handler)
				throw event_already_set();

			this->handler = func;

			if (this->event_added)
				this->event_added();
		}

		template<typename... V> exported T operator()(V&&... paras) {
			std::unique_lock<std::mutex> lck(this->lock);

			if (this->handler)
				return this->handler(std::forward<V>(paras)...);
			else
				return T();
		}
	};
}
