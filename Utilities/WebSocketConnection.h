#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"

#include <vector>

namespace util {
	namespace net {
		class websocket_connection : public tcp_connection {
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
	
				uint8* currentBufferStart;
				bool ready;

				bool doHandshake();
				bool send(const uint8* data, word length, OpCodes opCode);
				void close(CloseCodes code);

				public:
					exported websocket_connection(socket&& socket);
					exported websocket_connection(websocket_connection&& other);
					exported virtual ~websocket_connection() override;
					exported websocket_connection& operator=(websocket_connection&& other);

					exported virtual std::vector<tcp_connection::message> read(word messagesToWaitFor = 0) override;
					exported virtual bool send(const uint8* data, word length) override;
					exported virtual bool sendParts() override;
					exported virtual void close() override;

					websocket_connection(const websocket_connection& other) = delete;
					websocket_connection& operator=(const websocket_connection& other) = delete;
		};
	}
}
