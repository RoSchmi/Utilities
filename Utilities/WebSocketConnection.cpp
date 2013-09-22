#include "WebSocketConnection.h"
#include "Misc.h"
#include "Cryptography.h"
#include "DataStream.h"
#include <mutex>
#include <cstring>
#include <stdexcept>

using namespace Utilities;
using namespace Utilities::Net;
using namespace std;

WebSocketConnection::WebSocketConnection(TCPServer* server, Socket& socket) : TCPConnection(server, socket) {
	this->messageLength = 0;
	this->ready = false;
}

WebSocketConnection::~WebSocketConnection() {

}

void WebSocketConnection::doHandshake() {
	if (!this->connected)
		return;

	uint16 start = 0, end = 0;
	DataStream keyAndMagic;
	DataStream response;
	uint8 hash[Cryptography::SHA1_LENGTH];
	string base64;
	bool found;
	
	this->bytesReceived = static_cast<uint16>(this->connection.read(this->buffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived));

	if (this->bytesReceived == 0) {
		TCPConnection::disconnect();
		return;
	}

	for (uint16 i = 0; i < this->bytesReceived - 19U; i++) {
		if (memcmp("Sec-WebSocket-Key: ", this->buffer + i, 19) == 0) {
			start = i + 19;
			end = start;

			while ((this->buffer[end] != '\r' || this->buffer[end + 1] != '\n') && end < this->bytesReceived)
				end++;

			if (end == this->bytesReceived) {
				TCPConnection::disconnect();
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
		TCPConnection::disconnect();
		return;
	}

	keyAndMagic.write(this->buffer + start, end - start);
	keyAndMagic.write("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
	Cryptography::SHA1(keyAndMagic.getBuffer(), 60, hash);
	base64 = Misc::base64Encode(hash, 20);

	response.write("HTTP/1.1 101 Switching Protocols\r\nUpgrade: WebSocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: ");
	response.write(base64.c_str());
	response.write("\r\n\r\n");
	
	if (this->connection.ensureWrite(response.getBuffer(), response.getLength(), 10) != response.getLength()) {
		TCPConnection::disconnect();
		return;
	}

	this->ready = true;
	this->bytesReceived = 0;
}

vector<TCPConnection::Message> WebSocketConnection::read(uint32 messagesToWaitFor) {
	vector<TCPConnection::Message> messages;

	if (!this->connected)
		return messages;

	do {
		uint8 bytes[2];
		uint8 FIN;
		uint8 RSV1;
		uint8 RSV2;
		uint8 RSV3;
		uint16 opCode;
		uint8 mask;
		uint16 length;
		uint32 i;
		uint16 received;
		uint8 headerEnd;
		uint8 maskBuffer[MASK_BYTES];
		uint8* payloadBuffer;
		uint8* dataBuffer; //used for websocket's framing. The unmasked data from the current frame is copied to the start of Connection.buffer. Connection.messageLength is then used to point to the end of the data copied to the start of Connection.buffer.

		if (this->ready == false) {
			this->doHandshake();
			return messages;
		}

		dataBuffer = this->buffer + this->messageLength;
		received = static_cast<uint16>(this->connection.read(dataBuffer + this->bytesReceived, TCPConnection::MESSAGE_MAX_SIZE - this->bytesReceived - this->messageLength));
		this->bytesReceived += received;
	
		if (this->bytesReceived == 0) {
			TCPConnection::disconnect();
			messages.push_back(Message(true));
			goto error;
		}

		while (this->bytesReceived > 0) {
			if (this->bytesReceived >= 2) {
				bytes[0] = dataBuffer[0];
				bytes[1] = dataBuffer[1];

				headerEnd = 2;
		
				FIN = bytes[0] >> 7 & 0x1;
				RSV1 = bytes[0] >> 6 & 0x1;
				RSV2 = bytes[0] >> 5 & 0x1;
				RSV3 = bytes[0] >> 4 & 0x1;
				opCode = bytes[0] & 0xF;

				mask = bytes[1] >> 7 & 0x1;
				length = bytes[1] & 0x7F;

				if (mask == false || RSV1 || RSV2 || RSV3) {
					this->disconnect(CloseCodes::ProtocalError);
					messages.push_back(Message(true));
					goto error;
				}

				if (length == 126) {
					if (this->bytesReceived < 4) {
						messages.push_back(Message(true));
						goto error;
					}
					length = Net::networkToHostInt16(*reinterpret_cast<uint16*>(dataBuffer + 2));
					headerEnd += 2;
				}
				else if (length == 127) { //we don't support messages this big
					this->disconnect(CloseCodes::MessageTooBig);
					messages.push_back(Message(true));
					goto error;
				}

				switch ((OpCodes)opCode) { //ping is handled by the application, not websocket
					case OpCodes::Text: 
						this->disconnect(CloseCodes::InvalidDataType);
						messages.push_back(Message(true));
						goto error;
					case OpCodes::Close: 
						this->disconnect(CloseCodes::Normal);
						messages.push_back(Message(true));
						goto error;
					case OpCodes::Pong:
						headerEnd += mask ? MASK_BYTES : 0;
						this->bytesReceived -= headerEnd;
						dataBuffer = this->buffer + headerEnd;
						continue;
					case OpCodes::Ping: 
						if (length <= 125) {
							bytes[0] = 128 | static_cast<uint8>(OpCodes::Pong);  
							if (this->connection.ensureWrite(bytes, 2, 10) != 2 || this->connection.ensureWrite(dataBuffer + headerEnd, length + 4, 10) != length + 4) {
								TCPConnection::disconnect();
								messages.push_back(Message(true));
								goto error;
							} 
							headerEnd += mask ? MASK_BYTES : 0;
							this->bytesReceived -= headerEnd;
							dataBuffer = this->buffer + headerEnd;
							continue;
						}
						else {
							this->disconnect(CloseCodes::MessageTooBig);
							messages.push_back(Message(true));
							goto error;
						}
					case OpCodes::Continuation:
					case OpCodes::Binary:  
						memcpy(maskBuffer, dataBuffer + headerEnd, MASK_BYTES);
						headerEnd += MASK_BYTES;
						payloadBuffer = dataBuffer + headerEnd;
					
						this->bytesReceived -= length + headerEnd;
						for (i = 0; i < length; i++)
							dataBuffer[i] = payloadBuffer[i] ^ maskBuffer[i % MASK_BYTES];

						if (FIN) {
							messages.push_back(Message(this->buffer, this->messageLength + length));
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
						this->disconnect(CloseCodes::ProtocalError);
						messages.push_back(Message(true));
						goto error;
				}
			}
		}
	} while (messages.size() < messagesToWaitFor);

	error:
		return messages;
}

bool WebSocketConnection::send(const uint8* data, uint16 length) {
	if (!this->connected)
		return false;

	return this->send(data, length, OpCodes::Binary);
}

bool WebSocketConnection::send(const uint8* data, uint16 length, OpCodes opCode) {
	if (!this->connected)
		return false;

	uint8 bytes[4];
	uint8 sendLength;

	bytes[0] = 128 | static_cast<uint8>(opCode);
	sendLength = sizeof(length);

	if (length <= 125) {
		bytes[1] = static_cast<uint8>(length);
		*reinterpret_cast<uint16*>(bytes + 2) = 0;
	}
	else {
		bytes[1] = 126;
		sendLength += 2;
		*(reinterpret_cast<uint16*>(bytes + 2)) = Net::hostToNetworkInt16(length);
	}

	if (this->connection.ensureWrite(bytes, sendLength, 10) != sendLength) 
		goto sendFailed;
	if (this->connection.ensureWrite(data, length, 10) != length)
		goto sendFailed;

	return true;
sendFailed:
	TCPConnection::disconnect();
	return false;
}

bool WebSocketConnection::sendParts() {
	if (!this->connected)
		return false;

	uint8 bytes[4];
	uint8 sendLength;
	vector<pair<uint8 const*, uint16>>::iterator i;
	uint32 totalLength = 0;
	
	for (i = this->messageParts.begin(); i != this->messageParts.end(); i++)
		totalLength += i->second;

	bytes[0] = 128 | static_cast<uint8>(OpCodes::Binary);
	sendLength = sizeof(uint16);

	if (totalLength <= 125) {
		bytes[1] = (uint8)totalLength;
		*(uint16*)(bytes + 2) = 0;
	}
	else if (totalLength <= 65536) {
		bytes[1] = 126;
		sendLength += 2;
		*(uint16*)(bytes + 2) = Net::hostToNetworkInt16(static_cast<int16>(totalLength));
	}
	else { // we dont support longer messages
		this->disconnect(CloseCodes::MessageTooBig);
		return false;	
	}

	if (this->connection.ensureWrite(bytes, sendLength, 10) != sendLength) 
		goto sendFailed;
		
	for (auto i : this->messageParts)
		if (this->connection.ensureWrite(i.first, i.second, 10) != i.second)
			goto sendFailed;

	this->messageParts.clear();

	return true;
sendFailed:
	TCPConnection::disconnect();
	return false;
}

void WebSocketConnection::disconnect(CloseCodes code) {
	if (!this->connected)
		return;

	this->send(reinterpret_cast<uint8*>(&code), sizeof(uint16), OpCodes::Close);
	
	TCPConnection::disconnect();
}
