#pragma once

#include "Common.h"

#include <string>
#include <array>

namespace util {
	namespace net {
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
		class exported socket {
			public:	
				static const uint16 ADDRESS_LENGTH = 16;

				enum class types {
					TCP
				};

				enum class families {
					IPV4,
					IPV6,
					IPAny
				};

				socket(families family, types type, std::string address, std::string port);
				socket(families family, types type, std::string port);
				socket(socket&& other);
				socket();
				socket& operator=(socket&& other);
				~socket();

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
				word read(uint8* buffer, word bufferSize);

				/**
				 * Write @a writeAmount bytes from @a toWrite to the stream
				 *
				 * @returns Number of bytes written
				 */
				word write(const uint8* toWrite, word writeAmount);

				/**
				 * @returns Address of the host on the other end of a socket
				 * returned from @a accept(). Is always an IPv6 address (for now),
				 * but will sometimes be an IPv4 mapped address
				 */
				std::array<uint8, socket::ADDRESS_LENGTH> getRemoteAddress() const;

				/**
				 * @returns true if socket is connected, false otherwise
				 */
				bool isConnected() const;

				/**
				 * @returns true if there is data available to be read on the socket, false otherwise.
				 */
				bool isDataAvailable() const;

				socket(const socket& other) = delete;
				socket& operator=(const socket& other) = delete;

			private:
				types type;
				families family;
				bool connected;
				std::array<uint8, socket::ADDRESS_LENGTH> remoteEndpointAddress;
			
				#ifdef WINDOWS
					uintptr rawSocket;
				#elif defined POSIX
					int rawSocket;
				#endif

				socket(families family, types type);
		};

		int16 hostToNetworkInt16(int16 value);
		int32 hostToNetworkInt32(int32 value);
		int64 hostToNetworkInt64(int64 value);
		int16 networkToHostInt16(int16 value);
		int32 networkToHostInt32(int32 value);
		int64 networkToHostInt64(int64 value);
	}
}
