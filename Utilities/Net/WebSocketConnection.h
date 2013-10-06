#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"

#include <vector>

namespace util {
	namespace net {
		class websocket_connection : public tcp_connection {
				enum class op_codes {
					CONTINUATION = 0x0,
					TEXT = 0x1,
					BINARY = 0x2,
					CLOSE = 0x8,
					PING = 0x9,
					PONG = 0xA
				};

				enum class close_codes : uint16 {
					NORMAL = 1000,
					SERVER_SHUTDOWN = 1001,
					PROTOCAL_ERROR = 1002,
					INVALID_DATA_TYPE = 1003,
					MESSAGE_TOO_BIG = 1009
				};
	
				uint8* buffer_start;
				bool ready;

				bool handshake();
				bool send(const uint8* data, word length, op_codes code);
				void close(close_codes code);

				public:
					exported websocket_connection(socket&& socket);
					exported websocket_connection(websocket_connection&& other);
					exported virtual ~websocket_connection() override;
					exported websocket_connection& operator=(websocket_connection&& other);

					exported virtual std::vector<tcp_connection::message> read(word wait_for = 0) override;
					exported virtual bool send(const uint8* data, word length) override;
					exported virtual bool send_queued() override;
					exported virtual void close() override;

					websocket_connection(const websocket_connection& other) = delete;
					websocket_connection& operator=(const websocket_connection& other) = delete;
		};
	}
}
