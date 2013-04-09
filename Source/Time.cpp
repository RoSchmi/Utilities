#include "Time.h"

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined POSIX
	#include <sys/time.h>
#endif

using namespace Utilities;

DateTime::DateTime() {
	this->milliseconds = DateTime::getMillisecondsSinceEpoch();
}

DateTime::DateTime(uint64 milliseconds) {
	this->milliseconds = milliseconds;
}

DateTime::DateTime(const DateTime& other) {
	*this = other;
}

DateTime& DateTime::operator=(const DateTime& other) {
	this->milliseconds = other.milliseconds;

	return *this;
}

DateTime& DateTime::operator=(uint64 milliseconds) {
	this->milliseconds = milliseconds;

	return *this;
}

DateTime::operator uint64() {
	return this->milliseconds;
}

uint64 DateTime::getMilliseconds() const {
	return this->milliseconds;
}

DateTime& DateTime::addMilliseconds(uint64 milliseconds) {
	this->milliseconds += milliseconds;
	return *this;
}

DateTime& DateTime::addSeconds(uint64 seconds) {
	this->milliseconds += seconds * 1000;
	return *this;
}

DateTime& DateTime::addMinutes(uint64 minutes) {
	this->milliseconds += minutes * 60 * 1000;
	return *this;
}

DateTime& DateTime::addHours(uint64 hours) {
	this->milliseconds += hours * 60 * 60 * 1000;
	return *this;
}

DateTime& DateTime::addDays(uint64 days) {
	this->milliseconds += days * 24 * 60 * 60 * 1000;
	return *this;
}

DateTime DateTime::now() {
	return DateTime(DateTime::getMillisecondsSinceEpoch());
}

uint64 DateTime::getMillisecondsSinceEpoch() {
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

	gettimeofday(&tv, nullptr);

	result = tv.tv_sec;
	result *= 1000000; // convert seconds to microseconds
	result += tv.tv_usec; // add microseconds
	result /= 1000; // convert to milliseconds
	return result;
#endif
}