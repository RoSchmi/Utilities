#pragma once

#include <string>

#include "Common.h"

namespace util {
	namespace misc {
		/**
		 * Encode @a dataLength bytes from @a data as a base64 ASCII string
		 */
		exported std::string base64_encode(const uint8* data, word length);

		/**
		 * @return true if @a str is a valid UTF-8 encoded string, false
		 * otherwise
		 *
		 * @warning Does NOT check normalization etc!
		 */
		exported bool is_string_utf8(const std::string& str);
	}
}
