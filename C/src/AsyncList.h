#ifndef INCLUDE_UTILITIES_ASYNCLIST
#define INCLUDE_UTILITIES_ASYNCLIST

#include "Common.h"
#include "List.h"
#include "Thread.h"

typedef struct AsyncList AsyncList;
typedef struct AsyncList_Iterator AsyncList_Iterator;

struct AsyncList {
	List* BaseList;
	SAL_Mutex Lock;
	AsyncList_Iterator* DefaultIterator;
};

struct AsyncList_Iterator {
	AsyncList* BaseList;
	List_Iterator BaseIterator;
};

/* Iterates over every item in list using the list's DefaultIterator method. Resets the iterator upon invocation. Used like a while loop. */ 
#define AsyncList_ForEach(current, list, type) AsyncList_ResetIterator((list)->DefaultIterator); while ((current) = (type)AsyncList_Iterate((list)->DefaultIterator))

exported AsyncList* AsyncList_New(List_ElementDisposer elementDisposer);
exported void AsyncList_Initialize(AsyncList* list, List_ElementDisposer elementDisposer);
exported void AsyncList_Free(AsyncList* self);
exported void AsyncList_Uninitialize(AsyncList* self);

exported void AsyncList_Append(AsyncList* self, void* data);
exported uint64 AsyncList_GetCount(AsyncList* self);

exported void* AsyncList_Iterate(AsyncList_Iterator* iterator);
exported void AsyncList_InitializeIterator(AsyncList_Iterator* iterator, AsyncList* list);
exported void AsyncList_ResetIterator(AsyncList_Iterator* iterator);

#endif
