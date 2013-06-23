#include "DataStream.h"
#include "Misc.h"
#include <cstring>

using namespace Utilities;

DataStream::DataStream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	this->allocation = DataStream::MINIMUM_SIZE;
	this->buffer = new uint8[DataStream::MINIMUM_SIZE];
}

DataStream::DataStream(const uint8* exisitingBuffer, uint32 length) {
	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = new uint8[length];
	memcpy(this->buffer, exisitingBuffer, length);
}

DataStream::DataStream(DataStream&& other) {
	this->buffer = nullptr;
	*this = std::move(other);
}

DataStream& DataStream::operator=(DataStream&& other) {
	if (this->buffer)
		delete [] this->buffer;

	this->cursor = other.cursor;
	this->farthestWrite = other.farthestWrite;
	this->allocation = other.allocation;
	this->buffer = other.buffer;
	
	other.cursor = 0;
	other.farthestWrite = 0;
	other.allocation = 0;
	other.buffer = nullptr;

	return *this;
}

DataStream::DataStream(const DataStream& other) {
	this->cursor = other.cursor;
	this->farthestWrite = other.farthestWrite;
	this->allocation = other.allocation;
	this->buffer = new uint8[this->allocation];
	memcpy(this->buffer, other.buffer, other.allocation);
}

DataStream::~DataStream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	delete [] this->buffer;
}

const uint8* DataStream::getBuffer() const {
	return this->buffer;
}

uint32 DataStream::getLength() const {
	return this->farthestWrite;
}


bool DataStream::getEOF() const {
	return this->cursor < this->farthestWrite;
}

void DataStream::resize(uint32 newSize) {
	uint32 actualsize;
	uint8* newData;

	actualsize = DataStream::MINIMUM_SIZE;
	while (actualsize < newSize)
		actualsize *= 2;

	if (actualsize != this->allocation) {
		newData = new uint8[actualsize];
		if (this->buffer) {
			memcpy(newData, this->buffer, newSize > this->allocation ? this->allocation : newSize);
			delete [] this->buffer;
		}
		this->buffer = newData;
	}
	
	this->allocation = actualsize;
}

void DataStream::reset() {
	this->cursor = 0;
	this->farthestWrite = 0;
}

void DataStream::seek(uint32 position) {
	if (position > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	this->cursor = position;
}

void DataStream::adopt(uint8* buffer, uint32 length) {
	if (this->buffer)
		delete [] this->buffer;

	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = buffer;
}

void DataStream::write(const uint8* data, uint32 count) {
	if (this->cursor + count >= this->allocation)
		this->resize(this->cursor + count);

	memcpy(this->buffer + this->cursor, data, count);

	this->cursor += count;

	if (this->cursor > this->farthestWrite)
		this->farthestWrite = this->cursor;
}

void DataStream::writeArray(const Array& toWrite) {
	this->write(toWrite.getBuffer(), toWrite.getSize());
}

void DataStream::writeString(const std::string& toWrite) {
	uint16 size;

	size = (uint16)toWrite.size();

	this->write((uint8*)&size, sizeof(size));
	this->write((uint8*)toWrite.data(), size);
}

void DataStream::writeDataStream(const DataStream& toWrite) {
	this->write(toWrite.getBuffer(), toWrite.getLength());
}

void DataStream::read(uint8* buffer, uint32 count) {
	if (count + this->cursor > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	memcpy(buffer, this->buffer + this->cursor, count);
	this->cursor += count;
}

uint8* DataStream::read(uint32 count) {
	uint8* result;

	if (count + this->cursor > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	result = this->buffer + this->cursor;
	this->cursor += count;

	return result;
}

Array DataStream::readArray(uint32 count) {
	if (count + this->cursor > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	Array result(this->buffer + this->cursor, count);
	this->cursor += count;

	return result;
}

std::string DataStream::readString() {
	uint16 length;
	std::string string;
	int8* stringData;

	if (this->cursor + sizeof(uint16) <= this->farthestWrite) {
		length = *(uint16*)this->read(2);

		if (this->cursor + length <= this->farthestWrite) {
			stringData = (int8*)this->read(length);
			string = std::string(stringData, length);
		}
		else {
			this->cursor -= sizeof(uint16);
			throw DataStream::ReadPastEndException();
		}
	}
	else {
		throw DataStream::ReadPastEndException();
	}

	return string;
}
