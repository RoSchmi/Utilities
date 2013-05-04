#ifndef INCLUDE_UTILITIES_ASYNCQUEUE
#define INCLUDE_UTILITIES_ASYNCQUEUE

#include "Common.h"
#include "Queue.h"
#include "Thread.h"

typedef struct {
	SAL_Mutex Lock;
	Queue* BaseQueue;
} AsyncQueue;

exported AsyncQueue* AsyncQueue_New(void);
exported void AsyncQueue_Initialize(AsyncQueue* queue);
exported void AsyncQueue_Free(AsyncQueue* self);
exported void AsyncQueue_Uninitialize(AsyncQueue* self);

exported void* AsyncQueue_Dequeue(AsyncQueue* self);
exported void AsyncQueue_Enqueue(AsyncQueue* self, void* toEnqueue);

#endif
