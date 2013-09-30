#include "misc.h"

using namespace std;

string util::misc::base64Encode(const uint8* data, word dataLength) {
	static cstr characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"; 
	static uint8 endTable[] = { 0, 2, 1 };

	string result;
	result.resize(4 * ((dataLength + 2) / 3));

	for (word i = 0, j = 0; i < dataLength;) {
		uint32 a = i < dataLength ? data[i++] : 0;
		uint32 b = i < dataLength ? data[i++] : 0;
		uint32 c = i < dataLength ? data[i++] : 0;
		uint32 triple = (a << 0x10) + (b << 0x08) + c;

		result[j++] = characters[(triple >> 3 * 6) & 0x3F];
		result[j++] = characters[(triple >> 2 * 6) & 0x3F];
		result[j++] = characters[(triple >> 1 * 6) & 0x3F];
		result[j++] = characters[(triple >> 0 * 6) & 0x3F];
	}

	for (uint8 i = 0; i < endTable[dataLength % 3]; i++)
		result[result.size() - 1 - i] = '=';

	return result;
}

bool util::misc::isStringUTF8(string str) {
	cstr bytes = str.data();

	for (uint8 i = 0, toFind = 0, byte = bytes[0]; i < static_cast<uint8>(str.size()); byte = bytes[++i]) {
		if (toFind == 0) {
			if ((byte >> 1) == 126)
				toFind = 5;
			else if ((byte >> 2) == 62)
				toFind = 4;
			else if ((byte >> 3) == 30)
				toFind = 3;
			else if ((byte >> 4) == 14)
				toFind = 2;
			else if ((byte >> 5) == 6)
				toFind = 1;
			else if ((byte >> 7) == 0)
				;
			else
				return false;
		}
		else {
			if ((byte >> 6) == 2)
				toFind--;
			else
				return false;
		}
	}

	return true;
}
