#include "TCPConnection.h"

#include <cstring>
#include <stdexcept>
#include <thread>

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

TCPConnection::TCPConnection() {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = false;
	this->buffer = nullptr;
}

TCPConnection::TCPConnection(std::string address, std::string port, void* state) : connection(Socket::Families::IPAny, Socket::Types::TCP, address, port) {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[TCPConnection::MESSAGE_MAX_SIZE];
}

TCPConnection::TCPConnection(Socket&& socket) : connection(std::move(socket)) {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[TCPConnection::MESSAGE_MAX_SIZE];
}

TCPConnection::TCPConnection(TCPConnection&& other) : connection(std::move(other.connection)) {
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	this->buffer = other.buffer;
	other.buffer = nullptr;
	other.connected = false;
}

TCPConnection& TCPConnection::operator = (TCPConnection&& other) {
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

TCPConnection::~TCPConnection() {
	this->close();

	if (this->buffer)
		delete[] this->buffer;
}

array<uint8, Socket::ADDRESS_LENGTH> TCPConnection::getAddress() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection.getRemoteAddress();
}

const Socket& TCPConnection::getBaseSocket() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection;
}

bool TCPConnection::isDataAvailable() const {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->connection.isDataAvailable();
}

vector<TCPConnection::Message> TCPConnection::read(word messagesToWaitFor) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	vector<TCPConnection::Message> messages;

	if (!this->connected)
		return messages;

	do {
		word received = this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived);
		this->bytesReceived += received;

		if (received == 0) {
			this->close();
			messages.emplace_back(true);
			return messages;
		}
	
		while (this->bytesReceived >= TCPConnection::MESSAGE_LENGTH_BYTES) {
			sword length = reinterpret_cast<uint16*>(this->buffer)[0];
			sword remaining = this->bytesReceived - TCPConnection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.emplace_back(this->buffer + TCPConnection::MESSAGE_LENGTH_BYTES, length);
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

bool TCPConnection::send(const uint8* buffer, word length) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&length), TCPConnection::MESSAGE_LENGTH_BYTES))
		return false;

	if (!this->ensureWrite(buffer, length))
		return false;

	return true;
}

void TCPConnection::addPart(const uint8* buffer, word length) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	this->messageParts.emplace_back(buffer, length);
}

bool TCPConnection::sendParts() {
	if (!this->connected)
		throw runtime_error("Not connected.");

	word totalLength = 0;

	for (auto& i : this->messageParts)
		totalLength += i.length;

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&totalLength), TCPConnection::MESSAGE_LENGTH_BYTES))
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

void TCPConnection::close() {
	if (!this->connected)
		return;

	this->connection.close();
	this->connected = false;
}

bool TCPConnection::ensureWrite(const uint8* toWrite, word writeAmount) {
	if (writeAmount == 0)
		return true;

	word sentSoFar = 0;
	for (word i = 0; i < 10 && sentSoFar < writeAmount; i++) {
		sentSoFar += this->connection.write(toWrite + sentSoFar, writeAmount - sentSoFar);

		this_thread::sleep_for(chrono::milliseconds(i * 50));
	}

	return sentSoFar == writeAmount;
}

TCPConnection::Message::Message(bool closed) {
	this->wasClosed = closed;
	this->length = 0;
	this->data = nullptr;
}

TCPConnection::Message::Message(const uint8* buffer, word length) {
	this->wasClosed = false;
	this->length = length;
	this->data = new uint8[length];
	memcpy(this->data, buffer, length);
}

TCPConnection::Message::~Message() {
	if (this->data)
		delete[] this->data;
}

TCPConnection::Message::Message(const TCPConnection::Message& other) {
	this->data = nullptr;
	*this = other;
}

TCPConnection::Message::Message(TCPConnection::Message&& other) {
	this->data = nullptr;
	*this = std::move(other);
}

TCPConnection::Message& TCPConnection::Message::operator=(const TCPConnection::Message& other) {
	if (this->data)
		delete[] this->data;

	this->data = other.data;
	this->length = other.length;
	this->wasClosed = other.wasClosed;

	memcpy(this->data, other.data, this->length);

	return *this;
}


TCPConnection::Message& TCPConnection::Message::operator=(TCPConnection::Message&& other) {
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
