#pragma once

#include <string>

#include "Common.h"

namespace util {
	namespace misc {
		/**
		 * Encode @a dataLength bytes from @a data as a base64 ASCII string
		 */
		exported std::string base64Encode(const uint8* data, word dataLength);

		/**
		 * @return true if @a str is a valid UTF-8 encoded string, false
		 * otherwise
		 *
		 * @warning Does NOT check normalization etc!
		 */
		exported bool isStringUTF8(std::string str);
	}
}
