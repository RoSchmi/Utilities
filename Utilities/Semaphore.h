#pragma once

#include "Common.h"

#ifdef POSIX
	#include <semaphore.h>
#endif

namespace Utilities {
	class exported Semaphore {
		#ifdef WINDOWS
			void* baseSemaphore;
		#elif defined POSIX
			sem_t* baseSemaphore;
		#endif

		Semaphore(const Semaphore& other);
		Semaphore& operator=(const Semaphore& other);
		Semaphore(Semaphore&& other);
		Semaphore& operator=(Semaphore&& other);
	public:
		/// Possible return values for @a decrement().
		enum class DecrementResult {
			/// Decremented succesfully
			Success,
			/** Couldn't decrement the semaphore before the @a timeout
			 *  expired
			 */
			TimedOut,
			/// An error occured
			Error
		};

		Semaphore();
		~Semaphore();

		/**
		 * Increment this semaphore
		 */
		void increment();

		/**
		 * Decrement (consume) this semaphore
		 *
		 * @param timeout Number of milliseconds to wait (min) before returning
		 * DecrementResult::TimedOut
		 */
		DecrementResult decrement(uint32 timeout);
	};
}
