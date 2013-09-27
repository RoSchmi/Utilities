#pragma once

#include "Common.h"

#include <string>
#include <array>

namespace Utilities {
	namespace Net {
		/**
		 * Socket, a bidirectional stream endpoint
		 *
		 * General usage:
		 *
		 * Socket foo(Socket::Families::FooFamily, Socket::Types::FooProto);
		 * foo.connect("example.com", "80");
		 * // read/write here
		 *
		 * Or:
		 *
		 * Socket foo(Socket::Families::FooFamily, Socket::Types::FooProto);
		 * foo.listen("8080");
		 * foo.accept();
		 */
		class exported Socket {
			public:	
				static const uint16 ADDRESS_LENGTH = 16;

				enum class Types {
					TCP
				};

				enum class Families {
					IPV4,
					IPV6,
					IPAny
				};

				Socket(Families family, Types type, std::string address, std::string port);
				Socket(Families family, Types type, std::string port);
				Socket(Socket&& other);
				Socket& operator=(Socket&& other);
				~Socket();

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
				Socket accept();

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
				std::array<uint8, Socket::ADDRESS_LENGTH> getRemoteAddress() const;

				/**
				 * @returns true if socket is connected, false otherwise
				 */
				bool isConnected() const;

				/**
				 * @returns true if there is data available to be read on the socket, false otherwise.
				 */
				bool isDataAvailable() const;

				Socket() = delete;
				Socket(const Socket& other) = delete;
				Socket& operator=(const Socket& other) = delete;

			private:
				Types type;
				Families family;
				bool connected;
				std::array<uint8, Socket::ADDRESS_LENGTH> remoteEndpointAddress;
			
				#ifdef WINDOWS
					uintptr rawSocket;
				#elif defined POSIX
					int rawSocket;
				#endif

				Socket(Families family, Types type);
		};

		int16 hostToNetworkInt16(int16 value);
		int32 hostToNetworkInt32(int32 value);
		int64 hostToNetworkInt64(int64 value);
		int16 networkToHostInt16(int16 value);
		int32 networkToHostInt32(int32 value);
		int64 networkToHostInt64(int64 value);
	}
}
