#include "WebSocketConnection.h"

#include "Misc.h"
#include "Cryptography.h"
#include "DataStream.h"

#include <cstring>
#include <stdexcept>

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

WebSocketConnection::WebSocketConnection(Socket& socket) : TCPConnection(socket) {
	this->messageLength = 0;
	this->ready = false;
}

WebSocketConnection::WebSocketConnection(WebSocketConnection&& other) : TCPConnection(std::move(other)) {
	this->messageLength = other.messageLength;
	this->ready = other.ready;
}

WebSocketConnection& WebSocketConnection::operator = (WebSocketConnection&& other) {
	dynamic_cast<TCPConnection&>(*this) = std::move(dynamic_cast<TCPConnection&>(other));
	this->messageLength = other.messageLength;
	this->ready = other.ready;
	return *this;
}

WebSocketConnection::~WebSocketConnection() {

}

void WebSocketConnection::doHandshake() {
	if (!this->connected)
		return;

	word start = 0, end = 0;
	DataStream keyAndMagic;
	DataStream response;
	uint8 hash[Cryptography::SHA1_LENGTH];
	string base64;
	bool found;
	
	this->bytesReceived = static_cast<uint16>(this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived));

	if (this->bytesReceived == 0) {
		TCPConnection::close();
		return;
	}

	for (uint16 i = 0; i < this->bytesReceived - 19U; i++) {
		if (memcmp("Sec-WebSocket-Key: ", this->buffer + i, 19) == 0) {
			start = i + 19;
			end = start;

			while ((this->buffer[end] != '\r' || this->buffer[end + 1] != '\n') && end < this->bytesReceived)
				end++;

			if (end == this->bytesReceived) {
				TCPConnection::close();
				return;
			}

			found = true;

			break;
		}
		else {
			found = false;
		}
	}

	if (end == this->bytesReceived) {
		TCPConnection::close();
		return;
	}

	keyAndMagic.write(this->buffer + start, end - start);
	keyAndMagic.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	Cryptography::SHA1(keyAndMagic.getBuffer(), 60, hash);
	base64 = Misc::base64Encode(hash, 20);

	response.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
	response.write(base64.c_str());
	response.write("\r\n\r\n");
	
	if (!this->ensureWrite(response.getBuffer(), response.getLength())) {
		TCPConnection::close();
		return;
	}

	this->ready = true;
	this->bytesReceived = 0;
}

vector<const TCPConnection::Message> WebSocketConnection::read(word messagesToWaitFor) {
	vector<const TCPConnection::Message> messages;

	if (!this->connected)
		return messages;

	do {
		uint8 FIN;
		uint8 RSV1;
		uint8 RSV2;
		uint8 RSV3;
		uint16 opCode;
		uint8 mask;
		uint16 length;
		word received;
		uint8 headerEnd;
		uint8* dataBuffer; //used for websocket's framing. The unmasked data from the current frame is copied to the start of connection.buffer. connection.messageLength is then used to point to the end of the data copied to the start of connection.buffer.

		if (this->ready == false) {
			this->doHandshake();
			return messages;
		}

		dataBuffer = this->buffer + this->messageLength;
		received = this->connection.read(dataBuffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived - this->messageLength);
		this->bytesReceived += received;
	
		if (this->bytesReceived == 0) {
			TCPConnection::close();
			goto close;
		}

		while (this->bytesReceived > 0) {
			if (this->bytesReceived >= 2) {
				headerEnd = 2;
		
				FIN = dataBuffer[0] >> 7 & 0x1;
				RSV1 = dataBuffer[0] >> 6 & 0x1;
				RSV2 = dataBuffer[0] >> 5 & 0x1;
				RSV3 = dataBuffer[0] >> 4 & 0x1;
				opCode = dataBuffer[0] & 0xF;

				mask = dataBuffer[1] >> 7 & 0x1;
				length = dataBuffer[1] & 0x7F;

				if (mask == false || RSV1 || RSV2 || RSV3) {
					this->close(CloseCodes::ProtocalError);
					goto close;
				}

				if (length == 126) {
					length = Net::networkToHostInt16(reinterpret_cast<uint16*>(dataBuffer + 2)[0]);
					headerEnd += 2;
				}
				else if (length == 127) { //we don't support messages this big
					this->close(CloseCodes::MessageTooBig);
					goto close;
				}

				switch ((OpCodes)opCode) { //ping is handled by the application, not websocket
					case OpCodes::Text: 
						this->close(CloseCodes::InvalidDataType);
						goto close;

					case OpCodes::Close: 
						this->close(CloseCodes::Normal);
						goto close;

					case OpCodes::Pong:
						headerEnd += mask ? MASK_BYTES : 0;
						this->bytesReceived -= headerEnd;
						dataBuffer = this->buffer + headerEnd;

						continue;

					case OpCodes::Ping: 
						if (length <= 125) {
							dataBuffer[0] = 128 | static_cast<uint8>(OpCodes::Pong);  

							if (!this->ensureWrite(dataBuffer, 2) || !this->ensureWrite(dataBuffer + headerEnd, length)) {
								TCPConnection::close();
								goto close;
							} 

							headerEnd += mask ? MASK_BYTES : 0;
							this->bytesReceived -= headerEnd;
							dataBuffer = this->buffer + headerEnd;

							continue;
						}
						else {
							this->close(CloseCodes::MessageTooBig);
							goto close;
						}

					case OpCodes::Continuation:
					case OpCodes::Binary:
						uint8 maskBuffer[MASK_BYTES];
						uint8* payloadBuffer;

						memcpy(maskBuffer, dataBuffer + headerEnd, MASK_BYTES);
						headerEnd += MASK_BYTES;
						payloadBuffer = dataBuffer + headerEnd;
					
						this->bytesReceived -= length + headerEnd;

						for (word i = 0; i < length; i++)
							dataBuffer[i] = payloadBuffer[i] ^ maskBuffer[i % MASK_BYTES];

						if (FIN) {
							messages.emplace_back(this->buffer, this->messageLength + length);
							memcpy(this->buffer, this->buffer + length + headerEnd, this->bytesReceived);
							dataBuffer = this->buffer;
							this->messageLength = 0;
						}
						else {
							this->messageLength += length;
							dataBuffer = this->buffer + this->messageLength;
							memcpy(dataBuffer, payloadBuffer + length, this->bytesReceived);
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
		return false;

	return this->send(data, length, OpCodes::Binary);
}

bool WebSocketConnection::send(const uint8* data, word length, OpCodes opCode) {
	if (!this->connected)
		return false;

	uint8 bytes[4];
	word sendLength;

	bytes[0] = 128 | static_cast<uint8>(opCode);
	sendLength = 2;

	if (length <= 125) {
		bytes[1] = static_cast<uint8>(length);
		reinterpret_cast<uint16*>(bytes + 2)[0] = 0;
	}
	else {
		bytes[1] = 126;
		sendLength += 2;
		reinterpret_cast<int16*>(bytes + 2)[0] = Net::hostToNetworkInt16(static_cast<int16>(length));
	}

	if (!this->ensureWrite(bytes, sendLength)) 
		goto sendFailed;
	if (!this->ensureWrite(data, length))
		goto sendFailed;

	return true;

sendFailed:
	TCPConnection::close();
	return false;
}

bool WebSocketConnection::sendParts() {
	if (!this->connected)
		return false;

	uint8 bytes[4];
	word sendLength;
	word totalLength = 0;
	
	for (auto& i : this->messageParts)
		totalLength += i.length;

	bytes[0] = 128 | static_cast<uint8>(OpCodes::Binary);
	sendLength = 2;

	if (totalLength <= 125) {
		bytes[1] = static_cast<uint8>(totalLength);
		reinterpret_cast<uint16*>(bytes + 2)[0] = 0;
	}
	else if (totalLength <= 65536) {
		bytes[1] = 126;
		sendLength += 2;
		reinterpret_cast<int16*>(bytes + 2)[0] = Net::hostToNetworkInt16(static_cast<int16>(totalLength));
	}
	else { // we dont support longer messages
		this->close(CloseCodes::MessageTooBig);
		return false;	
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
		return;

	this->send(reinterpret_cast<uint8*>(&code), sizeof(uint16), OpCodes::Close);
	
	TCPConnection::close();
}

void WebSocketConnection::close() {
	this->close(CloseCodes::ServerShutdown);
}