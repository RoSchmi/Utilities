#ifndef INCLUDE_UTILITIES_ASYNCHASHTABLE
#define INCLUDE_UTILITIES_ASYNCHASHTABLE

#include "Common.h"

typedef struct AsyncHashTable AsyncHashTable;

exported AsyncHashTable* AsyncHashTable_New();
exported void AsyncHashTable_Initialize(AsyncHashTable* table);
exported void AsyncHashTable_Free(AsyncHashTable* self);
exported void AsyncHashTable_Uninitialize(AsyncHashTable* self);

exported void* AsyncHashTable_Get(AsyncHashTable* self, uint8* key, uint32 keyLength, void** value, uint32* valueLength); //returns the value as well in case the length is already known.
exported void AsyncHashTable_Add(AsyncHashTable* self, uint8* key, uint32 keyLength, void* value, uint32 valueLength);
exported void AsyncHashTable_Remove(AsyncHashTable* self, uint8* key, uint32 keyLength);
exported void* AsyncHashTable_GetInt(AsyncHashTable* self, uint64 key, void** value, uint32* valueLength);
exported void AsyncHashTable_AddInt(AsyncHashTable* self, uint64 key, void* value, uint32 valueLength);
exported void AsyncHashTable_RemoveInt(AsyncHashTable* self, uint64 key);

#define AsyncHashTable_GetIntType(table, key, type) (type)AsyncHashTable_GetInt((table), (key), NULL, NULL)
#define AsyncHashTable_AddIntType(table, key, value) AsyncHashTable_AddInt((table), (key), (void*)(value), sizeof(value))
#define AsyncHashTable_GetType(table, key, type) (type)AsyncHashTable_Get((table), (uint8*)(key), sizeof(key), NULL, NULL)
#define AsyncHashTable_AddType(table, key, value) AsyncHashTable_Add((table), (uint8*)(key), sizeof(key), (void*)(value), sizeof(value))

#endif
