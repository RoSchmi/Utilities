#pragma once

#include "Common.h"

namespace util {
	/**
	 * Cryptographic utilities
	 */
	namespace crypto {
		static const word SHA512_LENGTH = 64;
		static const word SHA1_LENGTH = 20;
		
		/**
		 * Take SHA512 of @a length bytes from @a source
		 *
		 * @param hashOutput Buffer the hash will be written to
		 */
		exported void SHA512(const uint8* source, word length, uint8 hashOutput[SHA512_LENGTH]);

		/**
		 * Take SHA512 of @a length bytes from @a source
		 *
		 * @param hashOutput Buffer the hash will be written to
		 */
		exported void SHA1(const uint8* source, word length, uint8 hashOutput[SHA1_LENGTH]);

		/**
		 * Write @a count random bytes to @a buffer
		 */
		exported void randomBytes(uint8* buffer, word count);

		/**
		 * @returns a random 64-bit integer in the interval of [@a floor, @a
		 * ceiling]
		 */
		exported int64 randomInt64(int64 floor, int64 ceiling);

		/**
		* @returns a random unsigned 64-bit integer in the interval of [@a floor, @a
		* ceiling]
		*/
		exported uint64 randomUInt64(uint64 floor, uint64 ceiling);

		/**
		 * @returns a random 64-bit floating point number in the interval of [@a floor, @a
		 * ceiling]
		 */
		exported float64 randomFloat64(float64 floor, float64 ceiling);
	}
}
