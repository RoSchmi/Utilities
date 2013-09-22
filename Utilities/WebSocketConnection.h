#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"
#include <vector>

namespace Utilities {
	namespace Net {
		class WebSocketConnection : public TCPConnection {
				enum class OpCodes {
					Continuation = 0x0,
					Text = 0x1,
					Binary = 0x2,
					Close = 0x8,
					Ping = 0x9,
					Pong = 0xA
				};

				enum class CloseCodes {
					Normal = 1000,
					ServerShutdown = 1001,
					ProtocalError = 1002,
					InvalidDataType = 1003,
					MessageTooBig = 1009
				};
	
				static const uint8 HEADER_LINES = 25;
				static const uint8 MASK_BYTES = 4;

				uint16 messageLength;
				bool ready;
			
				WebSocketConnection(TCPServer* server, Socket& socket);

				void doHandshake();
				bool send(const uint8* data, uint16 length, OpCodes opCode);
				void close(CloseCodes code);

				public:
					exported virtual std::vector<const TCPConnection::Message> read(uint32 messagesToWaitFor = 0);
					exported virtual bool send(const uint8* data, uint16 length);
					exported virtual bool sendParts();

					WebSocketConnection(const WebSocketConnection& other) = delete;
					WebSocketConnection& operator=(const WebSocketConnection& other) = delete;
					WebSocketConnection(WebSocketConnection&& other) = delete;
					WebSocketConnection& operator=(WebSocketConnection&& other) = delete;

					friend class TCPServer;
		};
	}
}
