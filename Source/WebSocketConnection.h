#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"
#include "MovableList.h"
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

				uint32 messageLength;
				bool ready;

				WebSocketConnection(const WebSocketConnection& other);
				WebSocketConnection& operator=(const WebSocketConnection& other);
				WebSocketConnection(WebSocketConnection&& other);
				WebSocketConnection& operator=(WebSocketConnection&& other);
			
				WebSocketConnection(TCPServer* server, Socket& socket);

				void doHandshake();
				bool send(const uint8* data, uint16 length, OpCodes opCode);
				void disconnect(CloseCodes code);
				virtual MovableList<TCPConnection::Message> read(uint32 messagesToWaitFor = 0);
			
				friend class TCPServer;

				public:
					exported ~WebSocketConnection();
				 
					exported virtual bool send(const uint8* data, uint16 length);
					exported virtual bool sendParts();
		};
	}
}
