#ifndef INCLUDE_UTILITIES_ARRAY
#define INCLUDE_UTILITIES_ARRAY

#include "Common.h"

typedef struct {
    uint8* Data;
    uint32 Size;
    uint32 Allocation;
} Array;

exported Array* Array_New(uint32 size);
exported Array* Array_NewFromExisting(uint8* data, uint32 size);
exported void Array_Initialize(Array* array, uint32 size);
exported void Array_Free(Array* self);
exported void Array_Uninitialize(Array* self);

exported void Array_Resize(Array* self, uint32 newSize);
exported uint8* Array_Read(Array* self, uint32 position, uint32 amount);
exported void Array_ReadTo(Array* self, uint32 position, uint32 amount, uint8* targetBuffer);
exported void Array_Write(Array* self, uint8* data, uint32 position, uint32 amount);
exported void Array_Append(Array* self, Array* source);

#endif
