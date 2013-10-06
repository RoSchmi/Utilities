#include "DataStream.h"

#include <cstring>

#include "Misc.h"

using namespace std;
using namespace util;

data_stream::data_stream() {
	this->cursor = 0;
	this->written = 0;
	this->allocation = data_stream::MINIMUM_SIZE;
	this->buffer = new uint8[data_stream::MINIMUM_SIZE];
}

data_stream::data_stream(uint8* data, word length) {
	this->cursor = 0;
	this->written = length;
	this->allocation = length;
	this->buffer = data;
}

data_stream::data_stream(const uint8* data, word length) {
	this->cursor = 0;
	this->written = length;
	this->allocation = length;
	this->buffer = new uint8[length];

	memcpy(this->buffer, data, length);
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
	this->written = 0;

	delete[] this->buffer;
}

data_stream& data_stream::operator=(data_stream&& other) {
	if (this->buffer)
		delete[] this->buffer;

	this->cursor = other.cursor;
	this->written = other.written;
	this->allocation = other.allocation;
	this->buffer = other.buffer;

	other.cursor = 0;
	other.written = 0;
	other.allocation = 0;
	other.buffer = nullptr;

	return *this;
}

data_stream& data_stream::operator=(const data_stream& other) {
	if (this->buffer)
		delete[] this->buffer;

	this->allocation = other.allocation;
	this->cursor = other.cursor;
	this->written = other.written;
	this->buffer = new uint8[this->allocation];

	memcpy(this->buffer, other.buffer, other.allocation);

	return *this;
}

const uint8* data_stream::data() const {
	return this->buffer;
}

const uint8* data_stream::data_at_cursor() const {
	return this->buffer + this->cursor;
}

word data_stream::size() const {
	return this->written;
}

word data_stream::position() const {
	return this->cursor;
}

void data_stream::resize(word size) {
	word new_allocation = data_stream::MINIMUM_SIZE;

	while (new_allocation < size)
		new_allocation *= data_stream::GROWTH;

	if (new_allocation != this->allocation) {
		uint8* new_buffer = new uint8[new_allocation];

		memcpy(new_buffer, this->buffer, size > this->allocation ? this->allocation : size);

		delete[] this->buffer;

		this->buffer = new_buffer;
		this->allocation = new_allocation;

		if (size < this->cursor)
			this->cursor = size;

		if (size < this->written)
			this->written = size;
	}
}

void data_stream::reset() {
	this->cursor = 0;
	this->written = 0;
}

void data_stream::seek(word position) {
	if (position > this->written)
		throw read_past_end_exception();

	this->cursor = position;
}

void data_stream::adopt(uint8* buffer, word length) {
	delete[] this->buffer;

	this->cursor = 0;
	this->written = length;
	this->allocation = length;
	this->buffer = buffer;
}

void data_stream::write(const uint8* data, word count) {
	if (this->cursor + count >= this->allocation)
		this->resize(this->cursor + count);

	memcpy(this->buffer + this->cursor, data, count);

	this->cursor += count;

	if (this->cursor > this->written)
		this->written = this->cursor;
}

void data_stream::write(const int8* data, word count) {
	this->write(reinterpret_cast<const uint8*>(data), count);
}

void data_stream::write(cstr data) {
	word size = static_cast<word>(strlen(data));

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(data), size);
}

void data_stream::write(const string& data) {
	word size = static_cast<word>(data.size());

	this->write(size);
	this->write(reinterpret_cast<const uint8*>(data.data()), size);
}

void data_stream::write(const data_stream& data) {
	this->write(data.data(), data.size());
}

void data_stream::write(const date_time& data) {
	this->write(since_epoch(data));
}

void data_stream::read(uint8* buffer, word count) {
	if (count + this->cursor > this->written)
		throw read_past_end_exception();

	memcpy(buffer, this->buffer + this->cursor, count);

	this->cursor += count;
}

const uint8* data_stream::read(word count) {
	if (count + this->cursor > this->written)
		throw read_past_end_exception();

	this->cursor += count;

	return this->buffer + this->cursor - count;
}

string data_stream::read_string() {
	word length = reinterpret_cast<word>(this->read(data_stream::STRING_LENGTH));

	return string(reinterpret_cast<cstr>(this->read(length)), length);
}

string data_stream::read_utf8() {
	string result = this->read_string();

	if (misc::is_string_utf8(result))
		throw string_not_utf8();

	return result;
}

date_time data_stream::read_date_time() {
	return util::from_epoch(this->read<uint64>());
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
	rhs = this->read_string();
	return *this;
}

data_stream& data_stream::operator>>(date_time& rhs) {
	rhs = this->read_date_time();
	return *this;
}