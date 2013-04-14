#include "TCPConnection.h"
#include "TCPServer.h"
#include <mutex>
#include <cstring>
#include <stdexcept>

using namespace Utilities::Net;
using namespace Utilities;
using namespace std;

TCPConnection::TCPConnection(std::string address, std::string port, void* state) : connection(Socket::Families::IPAny, Socket::Types::TCP, address, port) {
	this->owningServer = nullptr;
	this->bytesReceived = 0;
	this->state = state;
	this->connected = true;
}

TCPConnection::TCPConnection(TCPServer* server, Socket& socket) : connection(std::move(socket)) {
	this->owningServer = server;
	this->bytesReceived = 0;
	this->state = nullptr;
	this->connected = true;
} 

TCPConnection::~TCPConnection() {
	this->disconnect();
}

const Socket& TCPConnection::getBaseSocket() const {
	return this->connection;
}

MovableList<TCPConnection::Message> TCPConnection::read(uint32 messagesToWaitFor) {
	MovableList<TCPConnection::Message> messages;

	do {
		uint32 received = static_cast<uint32>(this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived));
		this->bytesReceived += received;

		if (received == 0) {
			this->disconnect();
			messages.insert(TCPConnection::Message(true));
		}
	
		while (this->bytesReceived >= TCPConnection::MESSAGE_LENGTH_BYTES) {
			uint16 length = *reinterpret_cast<uint16*>(this->buffer);
			int32 remaining = this->bytesReceived - TCPConnection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.insert(TCPConnection::Message(this->buffer + TCPConnection::MESSAGE_LENGTH_BYTES, length));
				memcpy(this->buffer, this->buffer + this->bytesReceived - remaining, remaining);
				this->bytesReceived = remaining;
			}
			else {
				break;
			}
		}
	} while (messages.getCount() < messagesToWaitFor);

	return messages;
}

bool TCPConnection::send(const uint8* buffer, uint16 length) {
	if (this->connection.ensureWrite(reinterpret_cast<uint8*>(&length), sizeof(length), 10) != sizeof(length))
		return false;

	if (this->connection.ensureWrite(buffer, length, 10) != length)
		return false;

	return true;
}

void TCPConnection::addPart(const uint8* buffer, uint16 length) {
	this->messageParts.push_back(make_pair(buffer, length));
}

bool TCPConnection::sendParts() {
	uint16 totalLength = 0;

	for (auto i : this->messageParts)
		totalLength += i.second;

	if (this->connection.ensureWrite((uint8*)&totalLength, sizeof(totalLength), 10) != sizeof(totalLength))
		goto error;
			
	for (auto i : this->messageParts)
		if (this->connection.ensureWrite(i.first, i.second, 10) != i.second)
			goto error;

	return true;

error:
	this->messageParts.clear();
	return false;
}

void TCPConnection::disconnect(bool callServerDisconnect) {
	if (!this->connected)
		return;

	this->connection.close();

	if (this->owningServer && callServerDisconnect)
		this->owningServer->onClientDisconnecting(this);

	this->connected = false;
}

TCPConnection::Message::Message(const uint8* buffer, uint16 length) {
	this->wasClosed = length == 0;
	this->length = length;
	if (length != 0) {
		this->data = new uint8[length];
		memcpy(this->data, buffer, length);
	}
	else {
		this->data = nullptr;
	}
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
	*this = std::move(other);
}

TCPConnection::Message& TCPConnection::Message::operator=(TCPConnection::Message&& other) {
	this->data = other.data;
	this->length = other.length;
	this->wasClosed = other.wasClosed;
	other.data = nullptr;
	other.length = 0;
	other.wasClosed = false;

	return *this;
}
