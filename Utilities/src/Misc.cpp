#include "Misc.h"

using namespace std;
using namespace util;

string misc::base64_encode(const uint8* data, word length) {
	static cstr characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	static uint8 endTable[] = { 0, 2, 1 };

	string result;
	result.resize(4 * ((length + 2) / 3));

	for (word i = 0, j = 0; i < length;) {
		uint8 a = i < length ? data[i++] : 0;
		uint8 b = i < length ? data[i++] : 0;
		uint8 c = i < length ? data[i++] : 0;
		uint32 triple = (a << 0x10) | (b << 0x08) | c;

		result[j++] = characters[(triple >> 3 * 6) & 0x3F];
		result[j++] = characters[(triple >> 2 * 6) & 0x3F];
		result[j++] = characters[(triple >> 1 * 6) & 0x3F];
		result[j++] = characters[(triple >> 0 * 6) & 0x3F];
	}

	for (word i = 0; i < endTable[length % 3]; i++)
		result[result.size() - 1 - i] = '=';

	return result;
}

bool misc::is_string_utf8(const string& str) {
	cstr bytes = str.data();

	uint8 needed = 0, current = bytes[0];
	for (word i = 0; i < str.size(); current = bytes[++i]) {
		if (needed == 0) {
			if ((current >> 1) == 126)
				needed = 5;
			else if ((current >> 2) == 62)
				needed = 4;
			else if ((current >> 3) == 30)
				needed = 3;
			else if ((current >> 4) == 14)
				needed = 2;
			else if ((current >> 5) == 6)
				needed = 1;
			else if ((current >> 7) == 0)
				;
			else
				return false;
		}
		else {
			if ((current >> 6) == 2)
				needed--;
			else
				return false;
		}
	}

	return true;
}
