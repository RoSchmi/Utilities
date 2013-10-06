#pragma once

#include <random>
#include <array>

#include "Common.h"

namespace util {
	/**
	 * Cryptographic utilities
	 */
	namespace crypto {
		static const word SHA2_LENGTH = 64;
		static const word SHA1_LENGTH = 20;

		extern std::random_device device;
		extern std::mt19937_64 generator;
		
		/**
		 * Take SHA2-512 of @a length bytes from @a source
		 */
		exported std::array<uint8, crypto::SHA2_LENGTH> calculate_sha2(const uint8* source, word length);

		/**
		 * Take SHA1 of @a length bytes from @a source
		 */
		exported std::array<uint8, crypto::SHA1_LENGTH> calculate_sha1(const uint8* source, word length);

		/**
		 * Write @a count random bytes to @a buffer
		 */
		exported void random_bytes(uint8* buffer, word count);

		/**
		 * @returns a random 64-bit integer in the interval of [@a floor, @a
		 * ceiling]
		 */
		exported int64 random_int64(int64 floor, int64 ceiling);

		/**
		* @returns a random unsigned 64-bit integer in the interval of [@a floor, @a
		* ceiling]
		*/
		exported uint64 random_uint64(uint64 floor, uint64 ceiling);

		/**
		 * @returns a random 64-bit floating point number in the interval of [@a floor, @a
		 * ceiling]
		 */
		exported float64 random_float64(float64 floor, float64 ceiling);
	}
}
