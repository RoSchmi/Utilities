#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>
#include <functional>

namespace util {
	template<typename C, typename T, typename... U> class event {
		public:
			typedef std::function<T(U...)> func_type;

			friend C;

			event() = default;
			~event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			std::function<void()> event_added;
			std::function<void()> event_removed;
			std::vector<func_type> registered;
			std::mutex lock;

			template<typename... V> std::vector<T> operator()(V&&... paras) {
				std::vector<T> results;
				std::unique_lock<std::mutex> lck(this->lock);

				for (auto i : this->registered)
					results.push_back(i(std::forward<V>(paras)...));

				return results;
			}

		public:
			event(event&& other) {
				*this = std::move(other);
			}

			event& operator=(event&& other) {
				std::unique_lock<std::mutex> lck1(this->lock);
				std::unique_lock<std::mutex> lck2(other.lock);

				this->registered = std::move(other.registered);

				return *this;
			}

			void operator+=(func_type hndlr) {
				std::unique_lock<std::mutex> lck(this->lock);

				this->registered.push_back(hndlr);

				if (this->event_added)
					this->event_added();
			}

			void operator-=(func_type hndlr) {
				std::unique_lock<std::mutex> lck(this->lock);

				auto pos = std::find(this->registered.begin(), this->registered.end(), hndlr);
				if (pos != this->registered.end())
					this->registered.erase(pos);

				if (this->event_removed)
					this->event_removed();
			}
	};

	template<typename C, typename... T> class event<C, void, T...> {
		public:
			typedef std::function<void(T...)> func_type;

			friend C;

			event() = default;
			~event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			std::function<void()> event_added;
			std::function<void()> event_removed;
			std::vector<func_type> registered;
			std::mutex lock;

			template<typename... U> void operator()(U&&... paras) {
				std::unique_lock<std::mutex> lck(this->lock);

				for (auto i : this->registered)
					i(std::forward<U>(paras)...);
			}

		public:
			event(event&& other) {
				*this = std::move(other);
			}

			event& operator=(event&& other) {
				std::unique_lock<std::mutex> lck1(this->lock);
				std::unique_lock<std::mutex> lck2(other.lock);

				this->registered = std::move(other.registered);

				return *this;
			}

			void operator+=(func_type hndlr) {
				std::unique_lock<std::mutex> lck(this->lock);

				this->registered.push_back(hndlr);

				if (this->event_added)
					this->event_added();
			}

			void operator-=(func_type hndlr) {
				std::unique_lock<std::mutex> lck(this->lock);

				auto pos = std::find(this->registered.begin(), this->registered.end(), hndlr);
				if (pos != this->registered.end())
					this->registered.erase(pos);

				if (this->event_removed)
					this->event_removed();
			}
	};

	template<typename C, typename T, typename... U> class event_single {
		public:
			typedef T(*ptr_type)(U...);
			typedef std::function<T(U...)> func_type;

			friend C;

			event_single() = default;
			~event_single() = default;
			event_single(const event_single& other) = delete;
			event_single& operator=(const event_single& other) = delete;

		private:
			std::function<void()> event_added;
			std::function<void()> event_removed;
			func_type registered;
			std::mutex lock;

			template<typename... V> T operator()(V&&... paras) {
				std::unique_lock<std::mutex> lck(this->lock);

				return this->registered(std::forward<V>(paras)...);
			}

		public:
			event_single(event_single&& other) {
				*this = std::move(other);
			}

			event_single& operator=(event_single&& other) {
				std::unique_lock<std::mutex> lck1(this->lock);
				std::unique_lock<std::mutex> lck2(other.lock);

				this->registered = other.registered;
				other.registered = nullptr;

				return *this;
			}

			void operator+=(func_type func) {
				std::unique_lock<std::mutex> lck(this->lock);

				this->registered = func;

				if (this->event_added)
					this->event_added();

			}

			void operator-=(func_type func) {
				std::unique_lock<std::mutex> lck(this->lock);

				if (!this->registered.target<ptr_type>() || !func.target<ptr_type>())
					return;

				if (*this->registered.target<ptr_type>() == *func.target<ptr_type>())
					this->registered = nullptr;

				if (this->event_removed)
					this->event_removed();
			}
	};
}
