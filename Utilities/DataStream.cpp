#include "DataStream.h"
#include "Misc.h"
#include <cstring>
#include <string.h>

using namespace Utilities;
using namespace std;

DataStream::DataStream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	this->allocation = DataStream::MINIMUM_SIZE;
	this->buffer = new uint8[DataStream::MINIMUM_SIZE];
}

DataStream::DataStream(const uint8* exisitingBuffer, word length) {
	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = new uint8[length];
	memcpy(this->buffer, exisitingBuffer, length);
}

DataStream::DataStream(DataStream&& other) {
	this->buffer = nullptr;
	*this = move(other);
}

DataStream::DataStream(const DataStream& other) {
	this->buffer = nullptr;
	*this = other;
}

DataStream::~DataStream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	delete [] this->buffer;
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

DataStream& DataStream::operator=(const DataStream& other) {
	if (this->buffer)
		delete [] this->buffer;

	this->allocation = other.allocation;
	this->cursor = other.cursor;
	this->farthestWrite = other.farthestWrite;
	this->buffer = new uint8[this->allocation];

	memcpy(this->buffer, other.buffer, other.allocation);

	return *this;
}

const uint8* DataStream::getBuffer() const {
	return this->buffer;
}

const uint8* DataStream::getBufferAtCursor() const {
	return this->buffer + this->cursor;
}

word DataStream::getLength() const {
	return this->farthestWrite;
}

bool DataStream::isEOF() const {
	return this->cursor < this->farthestWrite;
}

void DataStream::resize(word newSize) {
	word actualsize;
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

void DataStream::seek(word position) {
	if (position > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	this->cursor = position;
}

void DataStream::adopt(uint8* buffer, word length) {
	if (this->buffer)
		delete [] this->buffer;

	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = buffer;
}

void DataStream::write(const uint8* data, word count) {
	if (this->cursor + count >= this->allocation)
		this->resize(this->cursor + count);

	memcpy(this->buffer + this->cursor, data, count);

	this->cursor += count;

	if (this->cursor > this->farthestWrite)
		this->farthestWrite = this->cursor;
}

void DataStream::write(const int8* data, word count) {
	this->write(reinterpret_cast<const uint8*>(data), count);
}

void DataStream::write(cstr toWrite) {
	uint16 size = static_cast<uint16>(strlen(toWrite));

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(toWrite), size);
}

void DataStream::write(const string& toWrite) {
	uint16 size = static_cast<uint16>(toWrite.size());

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(toWrite.data()), size);
}

void DataStream::write(const DataStream& toWrite) {
	this->write(toWrite.getBuffer(), toWrite.getLength());
}

void DataStream::write(const datetime& toWrite) {
	this->write(chrono::duration_cast<chrono::milliseconds>(epoch - toWrite).count());
}

void DataStream::read(uint8* buffer, word count) {
	if (count + this->cursor > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	memcpy(buffer, this->buffer + this->cursor, count);
	this->cursor += count;
}

const uint8* DataStream::read(word count) {
	if (count + this->cursor > this->farthestWrite)
		throw DataStream::ReadPastEndException();

	uint8* result = this->buffer + this->cursor;
	this->cursor += count;

	return result;
}

string DataStream::readString() {
	uint16 length;
	string result;

	if (this->cursor + sizeof(uint16) <= this->farthestWrite) {
		length = this->read<uint16>();

		if (this->cursor + length <= this->farthestWrite) {
			result = string(reinterpret_cast<const int8*>(this->getBufferAtCursor()), length);
		}
		else {
			this->cursor -= sizeof(uint16);
			throw DataStream::ReadPastEndException();
		}
	}
	else {
		throw DataStream::ReadPastEndException();
	}

	return result;
}

datetime DataStream::readTimePoint() {
	return epoch + chrono::milliseconds(this->read<uint64>());
}