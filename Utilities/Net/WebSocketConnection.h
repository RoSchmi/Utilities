#pragma once

#include <vector>

#include "../Common.h"
#include "Socket.h"
#include "TCPConnection.h"

namespace util {
	namespace net {
		class websocket_connection : public tcp_connection {
				enum class op_codes {
					continuation = 0x0,
					text = 0x1,
					binary = 0x2,
					close = 0x8,
					ping = 0x9,
					pong = 0xA
				};

				enum class close_codes : uint16 {
					normal = 1000,
					server_shutdown = 1001,
					protocal_error = 1002,
					invalid_data_type = 1003,
					message_too_big = 1009
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
