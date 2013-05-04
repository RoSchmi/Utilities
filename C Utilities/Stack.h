#ifndef INCLUDE_UTILITIES_STACK
#define INCLUDE_UTILITIES_STACK

#include "Common.h"
#include "Array.h"

typedef struct {
	Array Data;
	uint64 Pointer;
} Stack;

exported Stack* Stack_New(uint64 size);
exported void Stack_Initialize(Stack* stack, uint64 size);
exported void Stack_Free(Stack* self);
exported void Stack_Uninitialize(Stack* self);
exported void Stack_Push(Stack* self, void* value);
exported void* Stack_Pop(Stack* self);

#endif
