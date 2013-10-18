#include "WebSocketConnection.h"

#include <cstring>

#include "../Misc.h"
#include "../Cryptography.h"
#include "../DataStream.h"

using namespace std;
using namespace util;
using namespace util::net;

websocket_connection::websocket_connection(socket&& socket) : tcp_connection(move(socket)) {
	this->ready = false;
	this->buffer_start = this->buffer;
}

websocket_connection::websocket_connection(websocket_connection&& other) : tcp_connection(move(other)) {
	this->ready = other.ready;
	this->buffer_start = other.buffer_start;
}

websocket_connection& websocket_connection::operator = (websocket_connection&& other) {
	dynamic_cast<tcp_connection&>(*this) = move(dynamic_cast<tcp_connection&>(other));
	this->ready = other.ready;
	this->buffer_start = other.buffer_start;
	return *this;
}

websocket_connection::~websocket_connection() {

}

bool websocket_connection::handshake() {
	word received = this->connection.read(this->buffer + this->received, tcp_connection::message_max_size - this->received);
	this->received += received;

	if (received == 0)
		return true;

	word i = 0, keyPos = 0, keyEnd = 0;
	for (; i < this->received - 4; i++) {
		if (i < this->received - 18 && memcmp("Sec-WebSocket-Key:", this->buffer + i, 18) == 0)
			keyPos = 18;

		if (keyPos != 0 && keyEnd == 0 && this->buffer[i] == '\r' && this->buffer[i + 1] == '\n')
			keyEnd = i - 1;

		if (this->buffer[i] == '\r' && this->buffer[i + 1] == '\n' && this->buffer[i + 2] == '\r' && this->buffer[i + 2] == '\n' && keyPos != 0)
			goto complete;
	}

	return false;

complete:
	while (this->buffer[keyPos] == ' ' && keyPos < this->received)
		keyPos++;
	while (this->buffer[keyEnd] == ' ')
		keyEnd--;

	data_stream key;
	key.write(this->buffer + keyPos, keyEnd - keyPos);
	key.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);

	string base64 = misc::base64_encode(crypto::calculate_sha1(key.data(), key.size()).data(), crypto::sha1_length);

	data_stream response;
	response.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ", 57);
	response.write(base64.c_str(), base64.size());
	response.write("\r\n\r\n", 4);
	
	if (!this->ensure_write(response.data(), response.size()))
		return true;

	this->ready = true;
	this->received -= i + 4;

	return false;
}

vector<tcp_connection::message> websocket_connection::read(word wait_for) {
	if (!this->connected)
		throw tcp_connection::not_connected_exception();

	vector<tcp_connection::message> messages;

	if (this->ready == false) {
		if (this->handshake()) {
			tcp_connection::close();
			goto close;
		}

		return messages;
	}

	do {
		while (this->received > 0) {
			word received = this->connection.read(this->buffer_start + this->received, tcp_connection::message_max_size - this->received);
			this->received += received;

			if (received == 0) {
				tcp_connection::close();
				goto close;
			}

			if (this->received >= 2) {
				bool RSV1 = (this->buffer_start[0] >> 6 & 0x1) != 0;
				bool RSV2 = (this->buffer_start[0] >> 5 & 0x1) != 0;
				bool RSV3 = (this->buffer_start[0] >> 4 & 0x1) != 0;
				bool FIN = (this->buffer_start[0] >> 7 & 0x1) != 0;
				bool mask = (this->buffer_start[1] >> 7 & 0x1) != 0;
				uint8 code = this->buffer_start[0] & 0xF;
				uint16 length = this->buffer_start[1] & 0x7F;
				word header_end = 2;

				if (!mask || RSV1 || RSV2 || RSV3) {
					this->close(close_codes::protocal_error);
					goto close;
				}

				if (length == 126) {
					length = net::net_to_host_int16(reinterpret_cast<uint16*>(this->buffer_start)[1]);
					header_end += 2;
				}
				else if (length == 127) {
					this->close(close_codes::message_too_big);
					goto close;
				}

				auto mask_buffer = this->buffer_start + header_end;
				auto payload_buffer = this->buffer_start + header_end + 4;
				header_end += 4;

				if (this->received < header_end + length)
					break;

				switch (static_cast<op_codes>(code)) {
					case op_codes::text: 
						this->close(close_codes::invalid_data_type);
						goto close;

					case op_codes::close:
						this->close(close_codes::normal);
						goto close;

					case op_codes::pong:
						continue;

					case op_codes::ping: 
						if (length > 125) {
							this->close(close_codes::message_too_big);
							goto close;
						}

						this->buffer[0] = 128 | static_cast<uint8>(op_codes::pong);

						if (!this->ensure_write(this->buffer_start, header_end + length)) {
							tcp_connection::close();
							goto close;
						} 

						continue;

					case op_codes::continuation:
					case op_codes::binary:					
						for (word i = 0; i < length; i++)
							payload_buffer[i] ^= mask_buffer[i % 4];

						if (FIN) {
							messages.emplace_back(payload_buffer, length);
							memcpy(payload_buffer + length, this->buffer, this->received - length - header_end);
							this->buffer_start = this->buffer;
						}
						else {
							memcpy(this->buffer_start, this->buffer_start + header_end, this->received - header_end);
							this->buffer_start += header_end;
						}

						continue;

					default:
						this->close(close_codes::protocal_error);
						goto close;
				}
			}
		}
	} while (messages.size() < wait_for);

	return messages;

close:
	messages.emplace_back(true);
	return messages;
}

bool websocket_connection::send(const uint8* data, word length) {
	if (!this->connected)
		throw tcp_connection::not_connected_exception();

	return this->send(data, length, op_codes::binary);
}

bool websocket_connection::send(const uint8* data, word length, op_codes code) {
	if (!this->connected)
		throw tcp_connection::not_connected_exception();

	if (length > 0xFFFF)
		throw tcp_connection::message_too_long_exception();

	word send_length = 2;
	uint8 bytes[4];
	bytes[0] = 128 | static_cast<uint8>(code);

	if (length <= 125) {
		bytes[1] = static_cast<uint8>(length);
	}
	else {
		bytes[1] = 126;
		send_length += 2;
		reinterpret_cast<int16*>(bytes)[1] = net::host_to_net_int16(static_cast<int16>(length));
	}

	if (!this->ensure_write(bytes, send_length) || !this->ensure_write(data, length)) {
		tcp_connection::close();
		return false;
	}

	return true;
}

bool websocket_connection::send_queued() {
	if (!this->connected)
		throw tcp_connection::not_connected_exception();

	uint8 bytes[4];
	word send_length = 2;
	word length = 0;
	
	for (auto& i : this->queued)
		length += i.length;

	bytes[0] = 128 | static_cast<uint8>(op_codes::binary);

	if (length <= 125) {
		bytes[1] = static_cast<uint8>(length);
	}
	else if (length <= 0xFFFF) {
		bytes[1] = 126;
		send_length += 2;
		reinterpret_cast<int16*>(bytes)[1] = net::host_to_net_int16(static_cast<int16>(length));
	}
	else {
		throw tcp_connection::message_too_long_exception();
	}

	if (!this->ensure_write(bytes, send_length)) 
		goto sendFailed;
		
	for (auto& i : this->queued)
		if (!this->ensure_write(i.data, i.length))
			goto sendFailed;

	this->queued.clear();

	return true;

sendFailed:
	tcp_connection::close();

	return false;
}

void websocket_connection::close(close_codes code) {
	if (!this->connected)
		throw tcp_connection::not_connected_exception();

	this->send(reinterpret_cast<uint8*>(&code), sizeof(code), op_codes::close);
	
	tcp_connection::close();
}

void websocket_connection::close() {
	this->close(close_codes::server_shutdown);
}