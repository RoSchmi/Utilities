#ifndef INCLUDE_SAL_CRYPTOGRAPHY
#define INCLUDE_SAL_CRYPTOGRAPHY

#include "Common.h"

exported uint8* SAL_Cryptography_SHA512(uint8* source, uint32 length);
exported uint8* SAL_Cryptography_SHA1(uint8* source, uint32 length);

exported uint8* SAL_Cryptography_RandomBytes(uint64 count);

exported uint64 SAL_Cryptography_RandomUInt64(uint64 floor, uint64 ceiling);
exported uint32 SAL_Cryptography_RandomUInt32(uint32 floor, uint32 ceiling);
exported uint16 SAL_Cryptography_RandomUInt16(uint16 floor, uint16 ceiling);
exported uint8 SAL_Cryptography_RandomUInt8(uint8 floor, uint8 ceiling);

#endif
