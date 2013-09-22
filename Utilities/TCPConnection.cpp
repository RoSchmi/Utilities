#include "TCPConnection.h"
#include "TCPServer.h"
#include <mutex>
#include <cstring>
#include <stdexcept>

using namespace Utilities::Net;
using namespace Utilities;
using namespace std;

TCPConnection::TCPConnection(std::string address, std::string port, void* state) : connection(Socket::Families::IPAny, Socket::Types::TCP, address, port) {
	this->bytesReceived = 0;
	this->state = state;
	this->connected = true;
}

TCPConnection::TCPConnection(Socket& socket) : connection(std::move(socket)) {
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
}

TCPConnection::TCPConnection(TCPConnection&& other) : connection(std::move(other.connection)) {
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	memcpy(this->buffer, other.buffer, this->bytesReceived);
}

TCPConnection& TCPConnection::operator = (TCPConnection&& other) {
	this->close();

	this->connection = std::move(other.connection);
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	memcpy(this->buffer, other.buffer, this->bytesReceived);

	return *this;
}

TCPConnection::~TCPConnection() {
	this->close();
}

const uint8* TCPConnection::getAddress() const {
	return this->connection.getRemoteAddress();
}

const Socket& TCPConnection::getBaseSocket() const {
	return this->connection;
}

const bool TCPConnection::isDataAvailable() const {
	return this->connection.isDataAvailable();
}

vector<const TCPConnection::Message> TCPConnection::read(uint32 messagesToWaitFor) {
	vector<const TCPConnection::Message> messages;

	if (!this->connected)
		return messages;

	do {
		word received = this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived);
		this->bytesReceived += received;

		if (received == 0) {
			this->close();
			messages.push_back(TCPConnection::Message(true));
			return messages;
		}
	
		while (this->bytesReceived >= TCPConnection::MESSAGE_LENGTH_BYTES) {
			uint16 length = reinterpret_cast<uint16*>(this->buffer)[0];
			word remaining = this->bytesReceived - TCPConnection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.push_back(TCPConnection::Message(this->buffer + TCPConnection::MESSAGE_LENGTH_BYTES, length));
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

bool TCPConnection::send(const uint8* buffer, uint16 length) {
	if (!this->connected)
		return false;

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&length), sizeof(length)))
		return false;

	if (!this->ensureWrite(buffer, length))
		return false;

	return true;
}

void TCPConnection::addPart(const uint8* buffer, uint16 length) {
	this->messageParts.push_back(TCPConnection::Message(buffer, length));
}

bool TCPConnection::sendParts() {
	if (!this->connected)
		return false;

	uint16 totalLength = 0;

	for (auto& i : this->messageParts)
		totalLength += i.length;

	if (!this->ensureWrite(reinterpret_cast<uint8*>(&totalLength), sizeof(totalLength)))
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

bool TCPConnection::ensureWrite(const uint8* toWrite, uint64 writeAmount) {
	if (writeAmount == 0)
		return true;

	uint64 sentSoFar = 0;
	for (uint8 i = 0; i < 10 && sentSoFar < writeAmount; i++) {
		sentSoFar += this->connection.write(toWrite + sentSoFar, writeAmount - sentSoFar);

		this_thread::sleep_for(chrono::milliseconds(i * 50));
	}

	return sentSoFar == writeAmount;
}

TCPConnection::Message::Message(const uint8* buffer, uint16 length) {
	this->wasClosed = false;
	this->length = length;
	this->data = new uint8[length];
	memcpy(this->data, buffer, length);
}

TCPConnection::Message::Message(bool closed) {
	this->wasClosed = closed;
	this->length = 0;
	this->data = nullptr;
}

TCPConnection::Message::~Message() {
	if (this->data)
		delete[] this->data;
}

TCPConnection::Message::Message(TCPConnection::Message&& other) {
	this->data = nullptr;
	*this = std::move(other);
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
