#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>

namespace util {
	template<typename C, typename T, typename... U> class event {
		public:
			typedef T(*handler)(U...);

			friend C;

			event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			typedef void(*on_change)();

			on_change event_added;
			on_change event_removed;
			std::vector<handler> registered;
			std::mutex lock;

			template<typename... V> std::vector<T> operator()(V&&... paras) {
				std::vector<T> results;
				std::unique_lock<mutex> lck(this->lock);

				for (auto i : this->registered)
					results.push_back(i(std::forward<V>(paras)...));

				return results;
			}

		public:
			event(event&& other) {
				*this = std::move(other);
			}

			event& operator=(event&& other) {
				std::unique_lock<mutex> lck1(this->lock);
				std::unique_lock<mutex> lck2(other.lock);

				this->registered = std::move(other.registered);

				return *this;
			}

			void operator+=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				this->registered.push_back(hndlr);

				if (this->event_added)
					this->event_added();
			}

			void operator-=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				auto pos = find(this->registered.begin(), this->registered.end(), hndlr);
				if (pos != this->registered.end())
					this->registered.erase(pos);

				if (this->event_removed)
					this->event_removed();
			}
	};

	template<typename C, typename... T> class event<C, void, T...> {
		public:
			typedef void(*handler)(T...);

			friend C;

			event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			typedef void(*on_change)();

			on_change event_added;
			on_change event_removed;
			std::vector<handler> registered;
			std::mutex lock;

			template<typename... U> void operator()(U&&... paras) {
				std::unique_lock<mutex> lck(this->lock);

				for (auto i : this->registered)
					i(std::forward<U>(paras)...);
			}

		public:
			event(event&& other) {
				*this = std::move(other);
			}

			event& operator=(event&& other) {
				std::unique_lock<mutex> lck1(this->lock);
				std::unique_lock<mutex> lck2(other.lock);

				this->registered = std::move(other.registered);

				return *this;
			}

			void operator+=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				this->registered.push_back(hndlr);

				if (this->event_added)
					this->event_added();
			}

			void operator-=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				auto pos = find(this->registered.begin(), this->registered.end(), hndlr);
				if (pos != this->registered.end())
					this->registered.erase(pos);

				if (this->event_removed)
					this->event_removed();
			}
	};

	template<typename C, typename T, typename... U> class event_single {
		public:
			typedef T(*handler)(U...);

			friend C;

			event_single() = default;
			event_single(const event_single& other) = delete;
			event_single& operator=(const event_single& other) = delete;

		private:
			typedef void(*on_change)();

			on_change event_added;
			on_change event_removed;
			handler registered;
			std::mutex lock;

			template<typename... V> T operator()(V&&... paras) {
				std::unique_lock<mutex> lck(this->lock);

				return this->registered(std::forward<V>(paras)...);
			}

		public:
			event_single(event_single&& other) {
				*this = std::move(other);
			}

			event_single& operator=(event_single&& other) {
				std::unique_lock<mutex> lck1(this->lock);
				std::unique_lock<mutex> lck2(other.lock);

				this->registered = other.registered;
				other.registered = nullptr;

				return *this;
			}

			void operator+=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				this->registered = hndlr;

				if (this->event_added)
					this->event_added();
			}

			void operator-=(handler hndlr) {
				std::unique_lock<mutex> lck(this->lock);

				if (hndlr == this->registered)
					this->registered = nullptr;

				if (this->event_removed)
					this->event_removed();
			}
	};
}