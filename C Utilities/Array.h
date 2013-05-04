#ifndef INCLUDE_UTILITIES_ARRAY
#define INCLUDE_UTILITIES_ARRAY

#include "Common.h"

typedef struct {
    uint8* Data;
    uint64 Size;
    uint64 Allocation;
} Array;

exported Array* Array_New(uint64 size);
exported Array* Array_NewFromExisting(uint8* data, uint64 size);
exported void Array_Initialize(Array* array, uint64 size);
exported void Array_Free(Array* self);
exported void Array_Uninitialize(Array* self);

exported void Array_Resize(Array* self, uint64 newSize);
exported uint8* Array_Read(Array* self, uint64 position, uint64 amount);
exported void Array_ReadTo(Array* self, uint64 position, uint64 amount, uint8* targetBuffer);
exported void Array_Write(Array* self, uint8* data, uint64 position, uint64 amount);
exported void Array_Append(Array* self, Array* source);

#endif
