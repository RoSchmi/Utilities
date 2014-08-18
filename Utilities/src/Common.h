#pragma once

#if defined _WIN64 || defined _WIN32
#define WINDOWS
#define threadlocal __declspec(thread)
#elif defined __unix__
#define POSIX
#define threadlocal __thread // assume we have __thread. It's implemented by gcc, icc, and clang.
#else
#error Platform not supported.
#endif

// libstdc++ doesn't support make_unique, libc++ supports it only in the unreleased 3.4
#ifdef POSIX

#include <memory>
#include <type_traits>

template <typename T, typename... Args> std::unique_ptr<T> make_unique_helper(std::false_type, Args&&... args) {
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T, typename... Args> std::unique_ptr<T> make_unique_helper(std::true_type, Args&&... args) {
	static_assert(std::extent<T>::value == 0, "make_unique<T[N]>() is forbidden, please use make_unique<T[]>().");

	typedef typename std::remove_extent<T>::type U;

	return std::unique_ptr<T>(new U[sizeof...(Args)]{std::forward<Args>(args)...});
}

template <typename T, typename... Args> std::unique_ptr<T> make_unique(Args&&... args) {
	return make_unique_helper<T>(std::is_array<T>(), std::forward<Args>(args)...);
}

#endif

#include <chrono>

typedef long long int64;
typedef int int32;
typedef short int16;
typedef char int8;
typedef unsigned long long uint64;
typedef unsigned int uint32;
typedef unsigned short uint16;
typedef unsigned char uint8;
typedef double float64;
typedef float float32;
typedef std::chrono::system_clock::time_point date_time;

namespace util {
	extern date_time epoch;

	uint64 since_epoch(const date_time& dt);
	date_time from_epoch(uint64 val);
}

#ifdef WINDOWS

#if defined _WIN64
typedef uint64 word;
typedef int64 sword;
#else
typedef uint32 word;
typedef int32 sword;
#endif

#elif defined POSIX

#if defined __X86_64__
typedef uint64 word;
typedef int64 sword;
#else
typedef uint32 word;
typedef int32 sword;
#endif

#endif

typedef const char* cstr;
typedef word uintptr;
