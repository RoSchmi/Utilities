#pragma once

#include "Common.h"
#include "Socket.h"
#include <vector>

namespace Utilities {
	namespace Net {
		class TCPServer;

		/**
		 * An abstraction over a Socket, providing the illusion of asynchronicity
		 * by using threads. Also provides minimal framing, of the format:
		 *
		 * +-+-+-+-+-+-+-+-+-+-+-+
		 * | length  |  data     |
		 * +-+-+-+-+-+-+-+-+-+-+-+
		 *
		 * length is two bytes, data is length bytes.
		 *
		 * The callbacks are fairly obvious from their prototypes
		 */
		class TCPConnection {
			public:
				struct exported Message {
					uint16 length;
					uint8* data;
					bool wasClosed;
					
					Message(bool closed);
					Message(const uint8* buffer, uint16 length);
					Message(Message&& other);
					~Message();
					Message& operator=(Message&& other);

					Message() = delete;
					Message(const Message& other) = delete;
					Message& operator=(const Message& other) = delete;
				};


				exported TCPConnection(Socket& socket);
				exported TCPConnection(std::string address, std::string port, void* state = nullptr);
				exported TCPConnection(TCPConnection&& other);
				exported virtual ~TCPConnection();
				exported TCPConnection& operator=(TCPConnection&& other);

				exported const uint8* getAddress() const;
				exported const Socket& getBaseSocket() const;
				exported const bool isDataAvailable() const;
				exported virtual std::vector<const Message> read(uint32 messagesToWaitFor = 0);
				exported virtual bool send(const uint8* buffer, uint16 length);
				exported void addPart(const uint8* buffer, uint16 length);
				exported virtual bool sendParts();
				exported void close();

				TCPConnection() = delete;
				TCPConnection(const TCPConnection& other) = delete;
				TCPConnection& operator=(const TCPConnection& other) = delete;

				void* state;

			protected:
				static const word MESSAGE_LENGTH_BYTES = 2;
				static const word MESSAGE_MAX_SIZE = 0xFFFF + MESSAGE_LENGTH_BYTES;
			
				Socket connection;
				uint8 buffer[MESSAGE_MAX_SIZE];
				word bytesReceived;
				bool connected;
				std::vector<Message> messageParts;

				bool ensureWrite(const uint8* toWrite, uint64 writeAmount);
		};
	}
}
