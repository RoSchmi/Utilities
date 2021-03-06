#pragma once

#include <string>
#include <array>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

#include "../Common.h"
#include "../Event.h"
#include "../Timer.h"

namespace util {
	namespace net {
		class tcp_connection;

		struct endpoint {
			std::string address;
			std::string port;
			bool is_websocket;

			endpoint(std::string address, std::string port, bool is_websocket = false);
			endpoint(std::string port, bool is_websocket = false);
			endpoint();
		};

		/**
		 * socket, a bidirectional stream endpoint
		 *
		 * General usage:
		 *
		 * socket foo(socket::families::FooFamily, socket::types::FooProto);
		 * foo.connect("example.com", "80");
		 * // read/write here
		 *
		 * Or:
		 *
		 * socket foo(socket::families::FooFamily, socket::types::FooProto);
		 * foo.listen("8080");
		 * foo.accept();
		 */
		class socket {
			public:	
				static const uint16 address_length = 16;

				enum class types {
					tcp
				};

				enum class families {
					ipv4,
					ipv6,
					ip_any
				};

				socket(families family, types type, endpoint ep);
				socket(socket&& other);
				socket();
				~socket();

				socket& operator=(socket&& other);

				class not_connected_exception {};
				class could_not_listen_exception {};
				class could_not_connect_exception {};
				class could_not_create_exception {};
				class invalid_address_exception {};

				/**
				 * Disconnect and close the connection
				 */
				void close();

				/**
				 * Accept a connection on this socket
				 *
				 * @returns New socket connected to the host that connected
				 *
				 * @warning Must have already called listen()
				 */
				socket accept();

				/**
				 * Read up to @a bufferSize from the stream into @a buffer
				 *
				 * @returns Number of bytes read
				 */
				word read(uint8* buffer, word count);

				/**
				 * Write @a writeAmount bytes from @a toWrite to the stream
				 *
				 * @returns Number of bytes written
				 */
				word write(const uint8* buffer, word count);

				/**
				 * @returns Address of the host on the other end of a socket
				 * returned from @a accept(). Is always an IPv6 address (for now),
				 * but will sometimes be an IPv4 mapped address
				 */
				std::array<uint8, socket::address_length> remote_address() const;

				/**
				 * @returns true if socket is connected, false otherwise
				 */
				bool is_connected() const;

				/**
				 * @returns true if there is data available to be read on the socket, false otherwise.
				 */
				bool data_available() const;

				socket(const socket& other) = delete;
				socket& operator=(const socket& other) = delete;

			private:
				types type;
				families family;
				bool connected;
				std::array<uint8, socket::address_length> endpoint_address;
			
				#ifdef WINDOWS
				uintptr raw_socket;
				#elif defined POSIX
				int raw_socket;
				#endif

				friend class async_worker;

				socket(families family, types type);
		};

		class async_worker {
			std::vector<std::shared_ptr<tcp_connection>> connections;
			std::recursive_mutex lock;
			timer<> timer;
			word index;

			void tick();

			public:
				async_worker();

				void add(std::shared_ptr<tcp_connection> s);
				void remove(std::shared_ptr<tcp_connection> s);

				event_single<bool, std::shared_ptr<tcp_connection>> on_data;

				async_worker(const async_worker&) = delete;
				async_worker(async_worker&&) = delete;
				async_worker& operator==(const async_worker&) = delete;
				async_worker& operator==(async_worker&&) = delete;
		};

		int16 host_to_net_int16(int16 value);
		int32 host_to_net_int32(int32 value);
		int64 host_to_net_int64(int64 value);
		int16 net_to_host_int16(int16 value);
		int32 net_to_host_int32(int32 value);
		int64 net_to_host_int64(int64 value);
	}
}
