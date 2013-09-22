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

TCPConnection::TCPConnection(TCPConnection&& other) : connection(std::move(other.connection)) {
	this->connected = false;
	this->owningServer = other.owningServer;
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	memcpy(this->buffer, other.buffer, this->bytesReceived);
}

TCPConnection& TCPConnection::operator = (TCPConnection&& other) {
	this->disconnect();

	this->connection = std::move(other.connection);
	this->owningServer = other.owningServer;
	this->state = other.state;
	this->connected = other.connected;
	this->messageParts = std::move(other.messageParts);
	this->bytesReceived = other.bytesReceived;
	memcpy(this->buffer, other.buffer, this->bytesReceived);

	return *this;
}

void* TCPConnection::getState() const {
	return this->state;
}

const Socket& TCPConnection::getBaseSocket() const {
	return this->connection;
}

vector<TCPConnection::Message> TCPConnection::read(uint32 messagesToWaitFor) {
	vector<TCPConnection::Message> messages;

	if (!this->connected)
		return messages;

	do {
		uint16 received = static_cast<uint16>(this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived));
		this->bytesReceived += received;

		if (received == 0) {
			this->disconnect();
			messages.push_back(TCPConnection::Message(true));
		}
	
		while (this->bytesReceived >= TCPConnection::MESSAGE_LENGTH_BYTES) {
			uint16 length = *reinterpret_cast<uint16*>(this->buffer);
			int32 remaining = this->bytesReceived - TCPConnection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.push_back(TCPConnection::Message(this->buffer + TCPConnection::MESSAGE_LENGTH_BYTES, length));
				memcpy(this->buffer, this->buffer + this->bytesReceived - remaining, remaining);
				this->bytesReceived = static_cast<uint16>(remaining);
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
	if (!this->connected)
		return false;

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
	this->data = nullptr;
	this->length = 0;
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
