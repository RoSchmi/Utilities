#include "TCPConnection.h"

#include <cstring>
#include <thread>
#include <utility>

using namespace std;
using namespace util;
using namespace util::net;

tcp_connection::tcp_connection() {
	this->received = 0;
	this->state = nullptr;
	this->connected = false;
	this->buffer = nullptr;
}

tcp_connection::tcp_connection(std::string address, std::string port, void* state) : connection(socket::families::IPAny, socket::types::TCP, address, port) {
	this->received = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[tcp_connection::MESSAGE_MAX_SIZE];
}

tcp_connection::tcp_connection(socket&& sock) : connection(move(sock)) {
	this->received = 0;
	this->state = nullptr;
	this->connected = true;
	this->buffer = new uint8[tcp_connection::MESSAGE_MAX_SIZE];
}

tcp_connection::tcp_connection(tcp_connection&& other) : connection(move(other.connection)) {
	this->state = other.state;
	this->connected = other.connected;
	this->queued = move(other.queued);
	this->received = other.received;
	this->buffer = other.buffer;
	other.buffer = nullptr;
	other.connected = false;
}

tcp_connection& tcp_connection::operator = (tcp_connection&& other) {
	this->close();

	this->connection = move(other.connection);
	this->state = other.state;
	this->connected = other.connected;
	this->queued = move(other.queued);
	this->received = other.received;
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

array<uint8, socket::ADDRESS_LENGTH> tcp_connection::address() const {
	if (!this->connected)
		throw not_connected_exception();

	return this->connection.remote_address();
}

const socket& tcp_connection::base_socket() const {
	if (!this->connected)
		throw not_connected_exception();

	return this->connection;
}

bool tcp_connection::data_available() const {
	if (!this->connected)
		throw not_connected_exception();

	return this->connection.data_available();
}

vector<tcp_connection::message> tcp_connection::read(word wait_for) {
	if (!this->connected)
		throw not_connected_exception();

	vector<tcp_connection::message> messages;

	if (!this->connected)
		return messages;

	do {
		word received = this->connection.read(this->buffer + this->received, tcp_connection::MESSAGE_MAX_SIZE - this->received);
		this->received += received;

		if (received == 0) {
			this->close();
			messages.emplace_back(true);
			return messages;
		}
	
		while (this->received >= tcp_connection::MESSAGE_LENGTH_BYTES) {
			sword length = reinterpret_cast<uint16*>(this->buffer)[0];
			sword remaining = this->received - tcp_connection::MESSAGE_LENGTH_BYTES - length;

			if (remaining >= 0) {
				messages.emplace_back(this->buffer + tcp_connection::MESSAGE_LENGTH_BYTES, length);
				memcpy(this->buffer, this->buffer + this->received - remaining, remaining);
				this->received = remaining;
			}
			else {
				break;
			}
		}
	} while (messages.size() < wait_for);

	return messages;
}

bool tcp_connection::send(const uint8* buffer, word length) {
	if (!this->connected)
		throw not_connected_exception();

	if (length > 0xFFFF)
		throw message_too_long_exception();

	if (!this->ensure_write(reinterpret_cast<uint8*>(&length), tcp_connection::MESSAGE_LENGTH_BYTES))
		return false;

	if (!this->ensure_write(buffer, length))
		return false;

	return true;
}

void tcp_connection::enqueue(const uint8* buffer, word length) {
	if (!this->connected)
		throw not_connected_exception();

	this->queued.emplace_back(buffer, length);
}

void tcp_connection::clear_queued() {
	this->queued.clear();
}

bool tcp_connection::send_queued() {
	if (!this->connected)
		throw not_connected_exception();

	word length = 0;
	for (auto& i : this->queued)
		length += i.length;

	if (length > 0xFFFF)
		throw message_too_long_exception();

	if (!this->ensure_write(reinterpret_cast<uint8*>(&length), tcp_connection::MESSAGE_LENGTH_BYTES))
		goto error;
			
	for (auto& i : this->queued)
		if (!this->ensure_write(i.data, i.length))
			goto error;

	this->queued.clear();

	return true;

error:
	this->queued.clear();

	return false;
}

void tcp_connection::close() {
	if (!this->connected)
		return;

	this->connection.close();
	this->connected = false;
}

bool tcp_connection::ensure_write(const uint8* data, word count) {
	if (count == 0)
		return true;

	word sent = 0;
	for (word i = 0; i < 10 && sent < count; i++) {
		sent += this->connection.write(data + sent, count - sent);

		this_thread::sleep_for(chrono::milliseconds(i * 50));
	}

	return sent == count;
}

tcp_connection::message::message(bool closed) {
	this->closed = closed;
	this->length = 0;
	this->data = nullptr;
}

tcp_connection::message::message(const uint8* buffer, word length) {
	this->closed = false;
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
	*this = move(other);
}

tcp_connection::message& tcp_connection::message::operator=(const tcp_connection::message& other) {
	if (this->data)
		delete[] this->data;

	this->data = other.data;
	this->length = other.length;
	this->closed = other.closed;

	memcpy(this->data, other.data, this->length);

	return *this;
}


tcp_connection::message& tcp_connection::message::operator=(tcp_connection::message&& other) {
	if (this->data)
		delete [] this->data;

	this->data = other.data;
	this->length = other.length;
	this->closed = other.closed;

	other.data = nullptr;
	other.length = 0;
	other.closed = false;

	return *this;
}
