#ifndef INCLUDE_UTILITIES_QUEUE
#define INCLUDE_UTILITIES_QUEUE

#include "Common.h"
#include "LinkedList.h"

typedef struct {
	LinkedList Data;
} Queue;

exported Queue* Queue_New(void);
exported void Queue_Initialize(Queue* queue);
exported void Queue_Free(Queue* self);
exported void Queue_Uninitialize(Queue* self);

exported void* Queue_Dequeue(Queue* self);
exported void Queue_Enqueue(Queue* self, void* toEnqueue);

#endif
