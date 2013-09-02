#include "Semaphore.h"
#include <utility>

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#include <Windows.h>
#elif defined POSIX
	#include <time.h>
	#include <errno.h>
#endif

using namespace Utilities;

Semaphore::Semaphore() {
#ifdef WINDOWS
	this->baseSemaphore = CreateSemaphore(NULL, 0, 10000, NULL);
#elif defined POSIX
	this->baseSemaphore = new sem_t;
	sem_init(this->baseSemaphore, 0 /* shared between threads */, 0);
#endif
}

Semaphore::~Semaphore() {
#ifdef WINDOWS
	CloseHandle(this->baseSemaphore);
#elif defined POSIX
	sem_destroy(this->baseSemaphore);
	delete this->baseSemaphore;
#endif
}

Semaphore::Semaphore(Semaphore&& other) {
	*this = std::move(other);
}

Semaphore& Semaphore::operator=(Semaphore && other) {
	this->baseSemaphore = other.baseSemaphore;

	other.baseSemaphore = nullptr;

	return *this;
}

void Semaphore::increment() {
#ifdef WINDOWS
	ReleaseSemaphore(this->baseSemaphore, 1, NULL);
#elif defined POSIX
	sem_post(this->baseSemaphore);
#endif
}

Semaphore::DecrementResult Semaphore::decrement(uint32 timeout) {
#ifdef WINDOWS
	DWORD result = WaitForSingleObject(this->baseSemaphore, timeout);
	if (result == WAIT_TIMEOUT)
		return Semaphore::DecrementResult::TimedOut;
	else if (result == WAIT_OBJECT_0)
		return Semaphore::DecrementResult::Success;
	else
		return Semaphore::DecrementResult::Error;
#elif defined POSIX
	timespec ts;
	int result;
eintr_restart:
	clock_gettime(CLOCK_REALTIME, &ts); /* Fuck. Posix. Worst decision ever made was sem_timedwait. */
	ts.tv_nsec += timeout * 1000 * 1000 /* ms to ns */;

	if (ts.tv_nsec >= 1000 * 1000 * 1000 /* a second */) {
		ts.tv_sec++;
		ts.tv_nsec %= 1000 * 1000 * 1000;
	}

	result = sem_timedwait(this->baseSemaphore, &ts);

	if (result == -1 && errno == EINTR)
		goto eintr_restart;
	else if (result == -1 && errno == ETIMEDOUT)
		return Semaphore::DecrementResult::TimedOut;
	else if (result == -1)
		return Semaphore::DecrementResult::Error;
	else
		return Semaphore::DecrementResult::Success;
#endif
}
