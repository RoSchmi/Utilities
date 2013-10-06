#pragma once

#include <vector>
#include <algorithm>
#include <mutex>
#include <utility>

namespace util {
	template<typename T, typename... args> class event {
		public:
			typedef T(*handler)(args...);

		private:
			std::vector<handler> registered;
			std::mutex lock;

		public:
			std::vector<T> operator()(args&&... paras);
			void operator+=(handler hndlr);
			void operator-=(handler hndlr);
	};

	template<typename... args> class event<void, args...> {
		public:
			typedef void(*handler)(args...);

		private:
			std::vector<handler> registered;
			std::mutex lock;

		public:
			void operator()(args&&... paras);
			void operator+=(handler hndlr);
			void operator-=(handler hndlr);
	};
}

namespace util {
	using namespace std;

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