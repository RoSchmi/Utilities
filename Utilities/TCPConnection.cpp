#include "TCPConnection.h"

#include <cstring>
#include <stdexcept>
#include <thread>

using namespace std;
using namespace util;
using namespace util::net;

tcp_connection::tcp_connection() {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = false;
	this->buffer = nullptr;
}

tcp_connection::tcp_connection(std::string address, std::string port, void* state) : connection(socket::families::IPAny, socket::types::TCP, address, port) {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[tcp_connection::MESSAGE_MAX_SIZE];
}

tcp_connection::tcp_connection(socket&& socket) : connection(std::move(socket)) {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[tcp_connection::MESSAGE_MAX_SIZE];
}

tcp_connection::tcp_connection(tcp_connection&& other) : connection(std::move(other.connection)) {
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	this->buffer = other.buffer;
	other.buffer = nullptr;
	other.connected = false;
}

tcp_connection& tcp_connection::operator = (tcp_connection&& other) {
	this->close();

	this->connection = std::move(other.connection);
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	this->buffer = other.buffer;
	other.buffer = nullptr;
	other.connected = false;

	return *this;
}

tcp_connection::~tcp_connection() {
	this->close();

	if (this->buffer)
		delete[] this->buffer;
}

array<uint8, socket::ADDRESS_LENGTH> tcp_connection::getAddress() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection.getRemoteAddress();
}

const socket& tcp_connection::getBaseSocket() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection;
}

bool tcp_connection::isDataAvailable() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection.isDataAvailable();
}

vector<tcp_connection::message> tcp_connection::read(word messagesToWaitFor) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	vector<tcp_connection::message> messages;

	if (!this->connected)
		return messages;

	do {
		word received = this->connection.read(this->buffer + this->bytesReceived, tcp_connection::MESSAGE_MAX_SIZE - this->bytesReceived);
		this->bytesReceived += received;

		if (received == 0) {
			this->close();
			messages.emplace_back(true);
			return messages;
		}
	
		while (this->bytesReceived >= tcp_connection::MESSAGE_LENGTH_BYTES) {
			sword length = reinterpret_cast<uint16*>(this->buffer)[0];
			sword remaining = this->bytesReceived - tcp_connection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.emplace_back(this->buffer + tcp_connection::MESSAGE_LENGTH_BYTES, length);
				memcpy(this->buffer, this->buffer + this->bytesReceived - remaining, remaining);
				this->bytesReceived = remaining;
			}
			else {
				break;
			}
		}
	} while (messages.size() < messagesToWaitFor);

	return messages;
}

bool tcp_connection::send(const uint8* buffer, word length) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	if (length > 0xFFFF)
		throw runtime_error("Length of message cannot exceed 0xFFFF.");

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&length), tcp_connection::MESSAGE_LENGTH_BYTES))
		return false;

	if (!this->ensureWrite(buffer, length))
		return false;

	return true;
}

void tcp_connection::addPart(const uint8* buffer, word length) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	this->messageParts.emplace_back(buffer, length);
}

void tcp_connection::clearParts() {
	this->messageParts.clear();
}

bool tcp_connection::sendParts() {
	if (!this->connected)
		throw runtime_error("Not connected.");

	word totalLength = 0;
	for (auto& i : this->messageParts)
		totalLength += i.length;

	if (totalLength > 0xFFFF)
		throw runtime_error("Combined length of messages cannot exceed 0xFFFF.");

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&totalLength), tcp_connection::MESSAGE_LENGTH_BYTES))
		goto error;
			
	for (auto& i : this->messageParts)
		if (!this->ensureWrite(i.data, i.length))
			goto error;

	this->messageParts.clear();

	return true;

error:
	this->messageParts.clear();

	return false;
}

void tcp_connection::close() {
	if (!this->connected)
		return;

	this->connection.close();
	this->connected = false;
}

bool tcp_connection::ensureWrite(const uint8* toWrite, word writeAmount) {
	if (writeAmount == 0)
		return true;

	word sentSoFar = 0;
	for (word i = 0; i < 10 && sentSoFar < writeAmount; i++) {
		sentSoFar += this->connection.write(toWrite + sentSoFar, writeAmount - sentSoFar);

		this_thread::sleep_for(chrono::milliseconds(i * 50));
	}

	return sentSoFar == writeAmount;
}

tcp_connection::message::message(bool closed) {
	this->wasClosed = closed;
	this->length = 0;
	this->data = nullptr;
}

tcp_connection::message::message(const uint8* buffer, word length) {
	this->wasClosed = false;
	this->length = length;
	this->data = new uint8[length];
	memcpy(this->data, buffer, length);
}

tcp_connection::message::~message() {
	if (this->data)
		delete[] this->data;
}

tcp_connection::message::message(const tcp_connection::message& other) {
	this->data = nullptr;
	*this = other;
}

tcp_connection::message::message(tcp_connection::message&& other) {
	this->data = nullptr;
	*this = std::move(other);
}

tcp_connection::message& tcp_connection::message::operator=(const tcp_connection::message& other) {
	if (this->data)
		delete[] this->data;

	this->data = other.data;
	this->length = other.length;
	this->wasClosed = other.wasClosed;

	memcpy(this->data, other.data, this->length);

	return *this;
}


tcp_connection::message& tcp_connection::message::operator=(tcp_connection::message&& other) {
	if (this->data)
		delete [] this->data;

	this->data = other.data;
	this->length = other.length;
	this->wasClosed = other.wasClosed;

	other.data = nullptr;
	other.length = 0;
	other.wasClosed = false;

	return *this;
}
