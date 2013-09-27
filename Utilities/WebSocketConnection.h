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

				enum class CloseCodes : uint16 {
					Normal = 1000,
					ServerShutdown = 1001,
					ProtocalError = 1002,
					InvalidDataType = 1003,
					MessageTooBig = 1009
				};
	
				static const word HEADER_LINES = 25;
				static const word MASK_BYTES = 4;

				word messageLength;
				bool ready;
			

				void doHandshake();
				bool send(const uint8* data, word length, OpCodes opCode);
				void close(CloseCodes code);

				public:
					exported WebSocketConnection(Socket&& socket);
					exported WebSocketConnection(WebSocketConnection&& other);
					exported virtual ~WebSocketConnection();
					exported WebSocketConnection& operator=(WebSocketConnection&& other);

					exported virtual std::vector<const TCPConnection::Message> read(word messagesToWaitFor = 0);
					exported virtual bool send(const uint8* data, word length);
					exported virtual bool sendParts();
					exported virtual void close();

					WebSocketConnection(const WebSocketConnection& other) = delete;
					WebSocketConnection& operator=(const WebSocketConnection& other) = delete;
		};
	}
}
