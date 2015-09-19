#ifndef INCLUDE_UTILITIES_STRING
#define INCLUDE_UTILITIES_STRING

#include "Common.h"
#include "Array.h"

typedef struct {
	Array Data;
	uint16 Length;
} String;


exported String* String_New(uint16 size);
exported String* String_NewFromCString(int8* cString);
exported void String_Initialize(String* string, uint16 size);
exported void String_Free(String* self);
exported void String_Uninitialize(String* self);

exported void String_AppendCString(String* self, int8* cString);
exported void String_AppendBytes(String* self, int8* bytes, uint16 size);
exported void String_AppendString(String* self, String* source);

exported boolean String_IsUTF8(String* self);

#endif
