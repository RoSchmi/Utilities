#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>

namespace util {
	template<typename T, typename... args> class event {
		public:
			typedef T(*handler)(args...);

			event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			std::vector<handler> registered;
			std::mutex lock;

		public:
			event(event&& other);
			event& operator=(event&& other);

			std::vector<T> operator()(args&&... paras);
			void operator+=(handler hndlr);
			void operator-=(handler hndlr);
	};

	template<typename... args> class event<void, args...> {
		public:
			typedef void(*handler)(args...);

			event() = default;
			event(const event& other) = delete;
			event& operator=(const event& other) = delete;

		private:
			std::vector<handler> registered;
			std::mutex lock;

		public:
			event(event&& other);
			event& operator=(event&& other);

			void operator()(args&&... paras);
			void operator+=(handler hndlr);
			void operator-=(handler hndlr);
	};
}

namespace util {
	using namespace std;

	template<typename T, typename... args> event<T, args...>::event(event&& other) {
		*this = move(other);
	}

	template<typename T, typename... args> event<T, args...>& event<T, args...>::operator=(event&& other) {
		unique_lock<mutex> lck1(this->lock);
		unique_lock<mutex> lck2(other.lock);

		this->registered = move(other.registered);

		return *this;
	}

	template<typename T, typename... args> vector<T> event<T, args...>::operator()(args&&... paras) {
		vector<T> results;
		unique_lock<mutex> lck(this->lock);

		for (auto i : this->registered)
			results.push_back(i(forward<args>(paras)...));

		return results;
	}

	template<typename T, typename... args> void event<T, args...>::operator+=(handler hndlr) {
		unique_lock<mutex> lck(this->lock);

		this->registered.push_back(hndlr);
	}

	template<typename T, typename... args> void event<T, args...>::operator-=(handler hndlr) {
		unique_lock<mutex> lck(this->lock);

		auto pos = find(this->registered.begin(), this->registered.end(), hndlr);
		if (pos != this->registered.end())
			this->registered.erase(pos);
	}


	template<typename... args> event<void, args...>::event(event&& other) {
		*this = move(other);
	}

	template<typename... args> event<void, args...>& event<void, args...>::operator=(event&& other) {
		unique_lock<mutex> lck1(this->lock);
		unique_lock<mutex> lck2(other.lock);

		this->registered = move(other.registered);

		return *this;
	}

	template<typename... args> void event<void, args...>::operator()(args&&... paras) {
		unique_lock<mutex> lck(this->lock);

		for (auto i : this->registered)
			i(forward<args>(paras)...);
	}

	template<typename... args> void event<void, args...>::operator+=(handler hndlr) {
		unique_lock<mutex> lck(this->lock);

		this->registered.push_back(hndlr);
	}

	template<typename... args> void event<void, args...>::operator-=(handler hndlr) {
		unique_lock<mutex> lck(this->lock);

		auto pos = find(this->registered.begin(), this->registered.end(), hndlr);
		if (pos != this->registered.end())
			this->registered.erase(pos);
	}
}