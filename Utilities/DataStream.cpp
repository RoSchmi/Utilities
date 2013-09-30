#include "DataStream.h"

#include <cstring>
#include <string.h>

#include "Misc.h"

using namespace std;
using namespace util;

data_stream::data_stream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	this->allocation = data_stream::MINIMUM_SIZE;
	this->buffer = new uint8[data_stream::MINIMUM_SIZE];
}

data_stream::data_stream(uint8* exisitingBuffer, word length) {
	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = exisitingBuffer;
}

data_stream::data_stream(const uint8* exisitingBuffer, word length) {
	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = new uint8[length];
	memcpy(this->buffer, exisitingBuffer, length);
}

data_stream::data_stream(data_stream&& other) {
	this->buffer = nullptr;
	*this = move(other);
}

data_stream::data_stream(const data_stream& other) {
	this->buffer = nullptr;
	*this = other;
}

data_stream::~data_stream() {
	this->cursor = 0;
	this->farthestWrite = 0;
	delete[] this->buffer;
}

data_stream& data_stream::operator=(data_stream&& other) {
	if (this->buffer)
		delete[] this->buffer;

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

data_stream& data_stream::operator=(const data_stream& other) {
	if (this->buffer)
		delete[] this->buffer;

	this->allocation = other.allocation;
	this->cursor = other.cursor;
	this->farthestWrite = other.farthestWrite;
	this->buffer = new uint8[this->allocation];

	memcpy(this->buffer, other.buffer, other.allocation);

	return *this;
}

const uint8* data_stream::getBuffer() const {
	return this->buffer;
}

const uint8* data_stream::getBufferAtCursor() const {
	return this->buffer + this->cursor;
}

word data_stream::getLength() const {
	return this->farthestWrite;
}

bool data_stream::isEOF() const {
	return this->cursor < this->farthestWrite;
}

void data_stream::resize(word newSize) {
	word actualsize;
	uint8* newData;

	actualsize = data_stream::MINIMUM_SIZE;
	while (actualsize < newSize)
		actualsize *= 2;

	if (actualsize != this->allocation) {
		newData = new uint8[actualsize];
		if (this->buffer) {
			memcpy(newData, this->buffer, newSize > this->allocation ? this->allocation : newSize);
			delete[] this->buffer;
		}
		this->buffer = newData;
	}

	this->allocation = actualsize;
}

void data_stream::reset() {
	this->cursor = 0;
	this->farthestWrite = 0;
}

void data_stream::seek(word position) {
	if (position > this->farthestWrite)
		throw data_stream::read_past_end_exception();

	this->cursor = position;
}

void data_stream::adopt(uint8* buffer, word length) {
	if (this->buffer)
		delete[] this->buffer;

	this->cursor = 0;
	this->farthestWrite = length;
	this->allocation = length;
	this->buffer = buffer;
}

void data_stream::write(const uint8* data, word count) {
	if (this->cursor + count >= this->allocation)
		this->resize(this->cursor + count);

	memcpy(this->buffer + this->cursor, data, count);

	this->cursor += count;

	if (this->cursor > this->farthestWrite)
		this->farthestWrite = this->cursor;
}

void data_stream::write(const int8* data, word count) {
	this->write(reinterpret_cast<const uint8*>(data), count);
}

void data_stream::write(cstr toWrite) {
	uint16 size = static_cast<uint16>(strlen(toWrite));

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(toWrite), size);
}

void data_stream::write(const string& toWrite) {
	uint16 size = static_cast<uint16>(toWrite.size());

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(toWrite.data()), size);
}

void data_stream::write(const data_stream& toWrite) {
	this->write(toWrite.getBuffer(), toWrite.getLength());
}

void data_stream::write(const date_time& toWrite) {
	this->write(chrono::duration_cast<chrono::milliseconds>(epoch - toWrite).count());
}

void data_stream::read(uint8* buffer, word count) {
	if (count + this->cursor > this->farthestWrite)
		throw data_stream::read_past_end_exception();

	memcpy(buffer, this->buffer + this->cursor, count);
	this->cursor += count;
}

const uint8* data_stream::read(word count) {
	if (count + this->cursor > this->farthestWrite)
		throw data_stream::read_past_end_exception();

	uint8* result = this->buffer + this->cursor;
	this->cursor += count;

	return result;
}

string data_stream::readString() {
	uint16 length;
	string result;

	if (this->cursor + sizeof(uint16) <= this->farthestWrite) {
		length = this->read<uint16>();

		if (this->cursor + length <= this->farthestWrite) {
			result = string(reinterpret_cast<const int8*>(this->getBufferAtCursor()), length);
		}
		else {
			this->cursor -= sizeof(uint16);
			throw data_stream::read_past_end_exception();
		}
	}
	else {
		throw data_stream::read_past_end_exception();
	}

	return result;
}

date_time data_stream::readTimePoint() {
	return epoch + chrono::milliseconds(this->read<uint64>());
}

data_stream& data_stream::operator<<(cstr rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(const std::string& rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(const data_stream& rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(const date_time& rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator>>(std::string& rhs) {
	rhs = this->readString();
	return *this;
}

data_stream& data_stream::operator>>(date_time& rhs) {
	rhs = this->readTimePoint();
	return *this;
}

data_stream& data_stream::operator<<(bool rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(float32 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(float64 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(uint8 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(uint16 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(uint32 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(uint64 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(int8 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(int16 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(int32 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator<<(int64 rhs) {
	this->write(rhs);
	return *this;
}

data_stream& data_stream::operator>>(bool& rhs) {
	rhs = this->read<bool>();
	return *this;
}

data_stream& data_stream::operator>>(float32& rhs) {
	rhs = this->read<float32>();
	return *this;
}

data_stream& data_stream::operator>>(float64& rhs) {
	rhs = this->read<float64>();
	return *this;
}

data_stream& data_stream::operator>>(uint8& rhs) {
	rhs = this->read<uint8>();
	return *this;
}

data_stream& data_stream::operator>>(uint16& rhs) {
	rhs = this->read<uint16>();
	return *this;
}

data_stream& data_stream::operator>>(uint32& rhs) {
	rhs = this->read<uint32>();
	return *this;
}

data_stream& data_stream::operator>>(uint64& rhs) {
	rhs = this->read<uint64>();
	return *this;
}

data_stream& data_stream::operator>>(int8& rhs) {
	rhs = this->read<int8>();
	return *this;
}

data_stream& data_stream::operator>>(int16& rhs) {
	rhs = this->read<int16>();
	return *this;
}

data_stream& data_stream::operator>>(int32& rhs) {
	rhs = this->read<int32>();
	return *this;
}

data_stream& data_stream::operator>>(int64& rhs) {
	rhs = this->read<int64>();
	return *this;
}
