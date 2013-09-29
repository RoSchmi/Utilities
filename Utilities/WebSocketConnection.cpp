#include "WebSocketConnection.h"

#include "Misc.h"
#include "Cryptography.h"
#include "DataStream.h"

#include <cstring>
#include <stdexcept>

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

WebSocketConnection::WebSocketConnection(Socket&& socket) : TCPConnection(std::move(socket)) {
	this->ready = false;
	this->currentBufferStart = this->buffer;
}

WebSocketConnection::WebSocketConnection(WebSocketConnection&& other) : TCPConnection(std::move(other)) {
	this->ready = other.ready;
	this->currentBufferStart = other.currentBufferStart;
}

WebSocketConnection& WebSocketConnection::operator = (WebSocketConnection&& other) {
	dynamic_cast<TCPConnection&>(*this) = std::move(dynamic_cast<TCPConnection&>(other));
	this->ready = other.ready;
	this->currentBufferStart = other.currentBufferStart;
	return *this;
}

WebSocketConnection::~WebSocketConnection() {

}

bool WebSocketConnection::doHandshake() {
	word received = this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived);
	this->bytesReceived += received;

	if (received == 0)
		return true;

	word i = 0, keyPos = 0, keyEnd = 0;
	for (; i < this->bytesReceived - 4; i++) {
		if (i < this->bytesReceived - 18 && memcmp("Sec-WebSocket-Key:", this->buffer + i, 18) == 0)
			keyPos = 18;

		if (keyPos != 0 && keyEnd == 0 && this->buffer[i] == '\r' && this->buffer[i + 1] == '\n')
			keyEnd = i - 1;

		if (this->buffer[i] == '\r' && this->buffer[i + 1] == '\n' && this->buffer[i + 2] == '\r' && this->buffer[i + 2] == '\n' && keyPos != 0)
			goto complete;
	}

	return false;

complete:
	while (this->buffer[keyPos] == ' ' && keyPos < this->bytesReceived)
		keyPos++;
	while (this->buffer[keyEnd] == ' ')
		keyEnd--;

	DataStream keyAndMagic;
	DataStream response;
	uint8 hash[Cryptography::SHA1_LENGTH];
	string base64;

	keyAndMagic.write(this->buffer + keyPos, keyEnd - keyPos);
	keyAndMagic.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11", 36);
	Cryptography::SHA1(keyAndMagic.getBuffer(), keyAndMagic.getLength(), hash);
	base64 = Misc::base64Encode(hash, Cryptography::SHA1_LENGTH);

	response.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ", 57);
	response.write(base64.c_str(), base64.size());
	response.write("\r\n\r\n", 4);
	
	if (!this->ensureWrite(response.getBuffer(), response.getLength()))
		return true;

	this->ready = true;
	this->bytesReceived -= i + 4;

	return false;
}

vector<TCPConnection::Message> WebSocketConnection::read(word messagesToWaitFor) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	vector<TCPConnection::Message> messages;

	if (this->ready == false) {
		if (this->doHandshake()) {
			TCPConnection::close();
			goto close;
		}

		return messages;
	}

	do {
		while (this->bytesReceived > 0) {
			word received = this->connection.read(this->currentBufferStart + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived);
			this->bytesReceived += received;

			if (received == 0) {
				TCPConnection::close();
				goto close;
			}

			if (this->bytesReceived >= 2) {
				bool RSV1 = (this->currentBufferStart[0] >> 6 & 0x1) != 0;
				bool RSV2 = (this->currentBufferStart[0] >> 5 & 0x1) != 0;
				bool RSV3 = (this->currentBufferStart[0] >> 4 & 0x1) != 0;
				bool FIN = (this->currentBufferStart[0] >> 7 & 0x1) != 0;
				bool mask = (this->currentBufferStart[1] >> 7 & 0x1) != 0;
				uint8 opCode = this->currentBufferStart[0] & 0xF;
				uint16 length = this->currentBufferStart[1] & 0x7F;
				word headerEnd = 2;

				if (!mask || RSV1 || RSV2 || RSV3) {
					this->close(CloseCodes::ProtocalError);
					goto close;
				}

				if (length == 126) {
					length = Net::networkToHostInt16(reinterpret_cast<uint16*>(this->currentBufferStart)[1]);
					headerEnd += 2;
				}
				else if (length == 127) {
					this->close(CloseCodes::MessageTooBig);
					goto close;
				}

				auto maskBuffer = this->currentBufferStart + headerEnd;
				auto payloadBuffer = this->currentBufferStart + headerEnd + 4;
				headerEnd += 4;

				if (this->bytesReceived < headerEnd + length)
					break;

				switch (static_cast<OpCodes>(opCode)) {
					case OpCodes::Text: 
						this->close(CloseCodes::InvalidDataType);
						goto close;

					case OpCodes::Close: 
						this->close(CloseCodes::Normal);
						goto close;

					case OpCodes::Pong:
						continue;

					case OpCodes::Ping: 
						if (length > 125) {
							this->close(CloseCodes::MessageTooBig);
							goto close;
						}

						this->buffer[0] = 128 | static_cast<uint8>(OpCodes::Pong);

						if (!this->ensureWrite(this->currentBufferStart, headerEnd + length)) {
							TCPConnection::close();
							goto close;
						} 

						continue;

					case OpCodes::Continuation:
					case OpCodes::Binary:					
						for (word i = 0; i < length; i++)
							payloadBuffer[i] ^= maskBuffer[i % 4];

						if (FIN) {
							messages.emplace_back(payloadBuffer, length);
							memcpy(payloadBuffer + length, this->buffer, this->bytesReceived - length - headerEnd);
							this->currentBufferStart = this->buffer;
						}
						else {
							memcpy(this->currentBufferStart, this->currentBufferStart + headerEnd, this->bytesReceived - headerEnd);
							this->currentBufferStart += headerEnd;
						}

						continue;

					default:
						this->close(CloseCodes::ProtocalError);
						goto close;
				}
			}
		}
	} while (messages.size() < messagesToWaitFor);

	return messages;

close:
	messages.emplace_back(true);
	return messages;
}

bool WebSocketConnection::send(const uint8* data, word length) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	return this->send(data, length, OpCodes::Binary);
}

bool WebSocketConnection::send(const uint8* data, word length, OpCodes opCode) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	if (length > 0xFFFF)
		throw runtime_error("Length of message cannot exceed 0xFFFF.");

	word sendLength = 2;
	uint8 bytes[4];
	bytes[0] = 128 | static_cast<uint8>(opCode);

	if (length <= 125) {
		bytes[1] = static_cast<uint8>(length);
	}
	else {
		bytes[1] = 126;
		sendLength += 2;
		reinterpret_cast<int16*>(bytes)[1] = Net::hostToNetworkInt16(static_cast<int16>(length));
	}

	if (!this->ensureWrite(bytes, sendLength) || !this->ensureWrite(data, length)) {
		TCPConnection::close();
		return false;
	}

	return true;
}

bool WebSocketConnection::sendParts() {
	if (!this->connected)
		throw runtime_error("Not connected.");

	uint8 bytes[4];
	word sendLength = 2;
	word totalLength = 0;
	
	for (auto& i : this->messageParts)
		totalLength += i.length;

	bytes[0] = 128 | static_cast<uint8>(OpCodes::Binary);

	if (totalLength <= 125) {
		bytes[1] = static_cast<uint8>(totalLength);
	}
	else if (totalLength <= 0xFFFF) {
		bytes[1] = 126;
		sendLength += 2;
		reinterpret_cast<int16*>(bytes)[1] = Net::hostToNetworkInt16(static_cast<int16>(totalLength));
	}
	else {
		throw runtime_error("Combined length of messages cannot exceed 0xFFFF.");
	}

	if (!this->ensureWrite(bytes, sendLength)) 
		goto sendFailed;
		
	for (auto& i : this->messageParts)
		if (!this->ensureWrite(i.data, i.length))
			goto sendFailed;

	this->messageParts.clear();

	return true;

sendFailed:
	TCPConnection::close();

	return false;
}

void WebSocketConnection::close(CloseCodes code) {
	if (!this->connected)
		throw runtime_error("Not connected.");

	this->send(reinterpret_cast<uint8*>(&code), sizeof(code), OpCodes::Close);
	
	TCPConnection::close();
}

void WebSocketConnection::close() {
	this->close(CloseCodes::ServerShutdown);
}