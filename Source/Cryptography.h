#pragma once

#include "Common.h"

namespace Utilities {
	namespace Cryptography {
		static const uint8 SHA512_LENGTH = 64;
		static const uint8 SHA1_LENGTH = 20;
		
		/**
		 * Take SHA512 of @a length bytes from @a source
		 *
		 * @param hashOutput Array the hash will be written to
		 */
		exported void SHA512(const uint8* source, uint32 length, uint8 hashOutput[SHA512_LENGTH]);

		/**
		 * Take SHA512 of @a length bytes from @a source
		 *
		 * @param hashOutput Array the hash will be written to
		 */
		exported void SHA1(const uint8* source, uint32 length, uint8 hashOutput[SHA1_LENGTH]);

		/**
		 * Write @a count random bytes to @a buffer
		 */
		exported void randomBytes(uint8* buffer, uint32 count);

		/**
		 * @returns a random 64-bit integer in the interval of [@a floor, @a
		 * ceiling]
		 */
		exported uint64 randomInt64(int64 floor, int64 ceiling);
	}
}
