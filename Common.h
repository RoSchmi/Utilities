#pragma once

#if defined _WIN64 || defined _WIN32
	#define WINDOWS
	#define exported  __declspec(dllexport)
	#define threadlocal __declspec(thread)
	#define _CRT_SECURE_NO_WARNINGS
#elif defined __unix__
	#define POSIX
	#define exported __attribute__((visibility ("default")))
	// assume we have __thread. It's implemented by gcc, icc, and clang.
	#define threadlocal __thread
#endif

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
typedef const int8* cstr;

#ifdef WINDOWS

#if defined _WIN32 && !defined _WIN64	
typedef unsigned int uintptr;
#elif defined _WIN64
typedef unsigned long long uintptr;
#endif

#elif defined POSIX

#if defined __X86_64__
typedef unsigned long long uintptr;
#else
typedef unsigned int uintptr;
#endif

#endif
