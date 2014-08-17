#ifndef INCLUDE_UTILITIES_DATASTREAM
#define INCLUDE_UTILITIES_DATASTREAM

#include "Common.h"
#include "Array.h"
#include "Strings.h"

typedef struct {
	Array Data;
	uint32 Cursor;
	boolean IsEOF;
} DataStream;

exported DataStream* DataStream_New(uint32 allocation);
exported void DataStream_Initialize(DataStream* dataStream, uint32 allocation);
exported void DataStream_Free(DataStream* self);
exported void DataStream_Uninitialize(DataStream* self);

exported void DataStream_Seek(DataStream* self, uint32 position);

exported void DataStream_WriteInt8(DataStream* self, int8 data);
exported void DataStream_WriteInt16(DataStream* self, int16 data);
exported void DataStream_WriteInt32(DataStream* self, int32 data);
exported void DataStream_WriteInt64(DataStream* self, int64 data);
exported void DataStream_WriteUInt8(DataStream* self, uint8 data);
exported void DataStream_WriteUInt16(DataStream* self, uint16 data);
exported void DataStream_WriteUInt32(DataStream* self, uint32 data);
exported void DataStream_WriteUInt64(DataStream* self, uint64 data);
exported void DataStream_WriteFloat32(DataStream* self, float32 data);
exported void DataStream_WriteFloat64(DataStream* self, float64 data);
exported void DataStream_WriteBytes(DataStream* self, uint8* data, uint32 count, boolean disposeBytes);
exported void DataStream_WriteArray(DataStream* self, Array* array, boolean disposeArray);
exported void DataStream_WriteString(DataStream* self, String* string, boolean disposeString);

exported int8 DataStream_ReadInt8(DataStream* self);
exported int16 DataStream_ReadInt16(DataStream* self);
exported int32 DataStream_ReadInt32(DataStream* self);
exported int64 DataStream_ReadInt64(DataStream* self);
exported uint8 DataStream_ReadUInt8(DataStream* self);
exported uint16 DataStream_ReadUInt16(DataStream* self);
exported uint32 DataStream_ReadUInt32(DataStream* self);
exported uint64 DataStream_ReadUInt64(DataStream* self);
exported float32 DataStream_ReadFloat32(DataStream* self);
exported float64 DataStream_ReadFloat64(DataStream* self);
exported uint8* DataStream_ReadBytes(DataStream* self, uint32 count);
exported Array* DataStream_ReadArray(DataStream* self, uint32 count);
exported String* DataStream_ReadString(DataStream* self);

#endif
