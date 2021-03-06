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
