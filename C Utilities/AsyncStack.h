#ifndef INCLUDE_UTILITIES_ASYNCSTACK
#define INCLUDE_UTILITIES_ASYNCSTACK

#include "Common.h"
#include "Stack.h"
#include "Thread.h"

typedef struct {
	SAL_Mutex Lock;
	Stack* BaseStack;
} AsyncStack;

exported AsyncStack* AsyncStack_New(uint64 size);
exported void AsyncStack_Initialize(AsyncStack* stack, uint64 size);
exported void AsyncStack_Free(AsyncStack* self);
exported void AsyncStack_Uninitialize(AsyncStack* self);
exported void AsyncStack_PushUInt64(AsyncStack* self, uint64 value);
exported void AsyncStack_PushUInt32(AsyncStack* self, uint32 value);
exported void AsyncStack_PushUInt16(AsyncStack* self, uint16 value);
exported void AsyncStack_PushUInt8(AsyncStack* self, uint8 value);
exported void AsyncStack_PushInt64(AsyncStack* self, int64 value);
exported void AsyncStack_PushInt32(AsyncStack* self, int32 value);
exported void AsyncStack_PushInt16(AsyncStack* self, int16 value);
exported void AsyncStack_PushInt8(AsyncStack* self, int8 value);
exported uint64 AsyncStack_PopUInt64(AsyncStack* self);
exported uint32 AsyncStack_PopUInt32(AsyncStack* self);
exported uint16 AsyncStack_PopUInt16(AsyncStack* self);
exported uint8 AsyncStack_PopUInt8(AsyncStack* self);
exported int64 AsyncStack_PopInt64(AsyncStack* self);
exported int32 AsyncStack_PopInt32(AsyncStack* self);
exported int16 AsyncStack_PopInt16(AsyncStack* self);
exported int8 AsyncStack_PopInt8(AsyncStack* self);

#endif
