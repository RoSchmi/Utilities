#pragma once

#include "Common.h"
#include <map>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <string>
#include <atomic>
#include <algorithm>

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
				uint64 read(uint8* buffer, uint64 bufferSize);

				/**
				 * Write @a writeAmount bytes from @a toWrite to the stream
				 *
				 * @returns Number of bytes written
				 */
				uint64 write(const uint8* toWrite, uint64 writeAmount);

				/**
				 * Similar to @a write, but will make @a maxAttempts tries to
				 * write the data if not all of the data was written on the first
				 * attempt.
				 *
				 * If maxAttempts is 0, try forever (not recommended).
				 */
				uint64 ensureWrite(const uint8* toWrite, uint64 writeAmount, uint8 maxAttempts);

				/**
				 * @returns Address of the host on the other end of a socket
				 * returned from @a accept(). Is always an IPv6 address (for now),
				 * but will sometimes be an IPv4 mapped address
				 */
				const uint8* getRemoteAddress() const;

				/**
				 * @returns true if socket is connected, false otherwise
				 */
				bool isConnected() const;

			private:
				Types type;
				Families family;
				bool connected;
				uint8 lastError;
				uint8 remoteEndpointAddress[ADDRESS_LENGTH];
			
				#ifdef WINDOWS
					uintptr rawSocket;
				#elif defined POSIX
					int rawSocket;
				#endif

				bool prepareRawSocket(std::string address, std::string port, bool willListenOn, void** addressInfo);
				Socket(Families family, Types type);
			
				Socket(const Socket& other);
				Socket& operator=(const Socket& other);

				friend class SocketAsyncWorker;
		};
	
		class SocketAsyncWorker {
			public:
				typedef void (*ReadCallback)(const Socket& socket, void* state);
			
				exported SocketAsyncWorker(ReadCallback callback);
				exported ~SocketAsyncWorker();
				exported void registerSocket(const Socket& socket, void* state);
				exported void unregisterSocket(const Socket& socket);
				exported void shutdown();
				exported void start();

			private:
				ReadCallback callback;
				std::atomic<bool> running;
				std::thread worker;
				std::recursive_mutex listLock;
				std::map<const Socket*, void*> list;

				SocketAsyncWorker(const SocketAsyncWorker& other);
				SocketAsyncWorker& operator=(const SocketAsyncWorker& other);
				SocketAsyncWorker(SocketAsyncWorker&& other);
				SocketAsyncWorker& operator=(SocketAsyncWorker&& other);

				#ifdef POSIX
					int maxFD;
				#endif

				void run();
		};

		int16 hostToNetworkInt16(int16 value);
		int32 hostToNetworkInt32(int32 value);
		int64 hostToNetworkInt64(int64 value);
		int16 networkToHostInt16(int16 value);
		int32 networkToHostInt32(int32 value);
		int64 networkToHostInt64(int64 value);
	}
}
