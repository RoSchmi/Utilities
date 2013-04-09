#include "Array.h"
#include "Misc.h"
#include <cstring>
#include <cstdlib>
#include <utility>

using namespace Utilities;

Array::Array(uint32 size) {
	this->recalculateSize(size);
	this->buffer = static_cast<uint8*>(malloc(this->allocation));
	this->furthestWrite = size;
}

Array::Array(const uint8* data, uint32 size) {
	this->recalculateSize(size);
	this->buffer = static_cast<uint8*>(malloc(this->allocation));
	memcpy(this->buffer, data, size);
	this->furthestWrite = size;
}

Array::Array(const Array& other) {
	*this = other;
}

Array::Array(Array&& other) {
	*this = std::move(other);
}

Array& Array::operator=(const Array& other) {
	this->allocation = other.allocation;
	this->furthestWrite = other.furthestWrite;
	this->buffer = static_cast<uint8*>(malloc(this->allocation));
	memcpy(this->buffer, other.buffer, this->allocation);

	return *this;
}

Array& Array::operator=(Array&& other) {
	this->buffer = other.buffer;
	this->allocation = other.allocation;
	this->furthestWrite = other.furthestWrite;
	other.buffer = nullptr;
	other.allocation = 0;
	other.furthestWrite = 0;

	return *this;
}

Array::~Array() {
	if (this->buffer)
		free(this->buffer);
}

void Array::recalculateSize(uint32 givenSize) {
	this->allocation = Array::MINIMUM_SIZE;
	while (givenSize > this->allocation)
		this->allocation *= 4;
}

uint32 Array::getSize() const {
	return this->furthestWrite;
}

const uint8* Array::getBuffer() const {
	return this->buffer;
}

uint8* Array::getBuffer() {
	return this->buffer;
}

void Array::ensureSize(uint32 size) {
	if (this->furthestWrite < size)
		this->furthestWrite = size;
	this->resize(size);
}

void Array::resize(uint32 newSize) {
	this->recalculateSize(newSize);
	this->buffer = static_cast<uint8*>(realloc(this->buffer, this->allocation));
}

const uint8* Array::read(uint32 position, uint32 amount) {
	return this->buffer + position;
}

void Array::readTo(uint32 position, uint32 amount, uint8* targetBuffer) {
	memcpy(targetBuffer, this->buffer + position, amount);
}

void Array::write(const uint8* data, uint32 position, uint32 amount) {
	if (position + amount > this->allocation)
		this->resize(position + amount);
		
	if (this->furthestWrite < position + amount)
		this->furthestWrite = position + amount;

	memcpy(this->buffer + position, data, amount);
}

void Array::write(const int8* data, uint32 position, uint32 amount) {
	this->write(reinterpret_cast<const uint8*>(data), position, amount);
}

void Array::write(const Array& source, uint32 position, uint32 amount) {
	amount = amount != 0 ? amount : source.furthestWrite;

	if (position + amount > this->allocation)
		this->resize(position + amount);

	this->write(source.buffer, position, amount);
}
