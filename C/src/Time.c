#include "Time.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined POSIX
	#include <sys/time.h>
#endif

/**
 * @returns the current time in ms since Jan 1, 1970.
 *
 * @warning Does *NOT* take into account leap seconds, but does take into
 * account leap days.
 */
uint64 Time_Now(void) {
#ifdef WINDOWS
	FILETIME time;
	uint64 result;

	GetSystemTimeAsFileTime(&time);

	result = time.dwHighDateTime;
	result <<= 32;
	result += time.dwLowDateTime;
	result /= 10000; // to shift from 100ns intervals to 1ms intervals
	result -= 11644473600000; // to shift from epoch of 1/1/1601 to 1/1/1970

	return result;
#elif defined POSIX
	int64 result;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	result = tv.tv_sec;
	result *= 1000000; // convert seconds to microseconds
	result += tv.tv_usec; // add microseconds
	result /= 1000; // convert to milliseconds
	return (uint64)result;
#endif
}

uint64 Time_AddDays(uint64 time, int32 days) {
	return time + (days * 24 * 60 * 60 * 1000);
}

uint64 Time_AddHours(uint64 time, int32 hours) {
	return time + (hours * 60 * 60 * 1000);
}

uint64 Time_AddMinutes(uint64 time, int32 minutes) {
	return time + (minutes * 60 * 1000);
}

uint64 Time_AddSeconds(uint64 time, int32 seconds) {
	return time + (seconds * 1000);
}
