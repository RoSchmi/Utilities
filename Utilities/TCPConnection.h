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
					~Message();
					Message(Message&& other);
					Message& operator=(Message&& other);

					private:
						Message(const Message& other);
						Message& operator=(const Message& other);
				};

				exported TCPConnection();
				exported TCPConnection(std::string address, std::string port, void* state);
				exported virtual ~TCPConnection();

				exported TCPConnection(TCPConnection&& other);
				exported TCPConnection& operator=(TCPConnection&& other);

				exported void* getState() const;
				exported const Socket& getBaseSocket() const;
				exported virtual std::vector<Message> read(uint32 messagesToWaitFor = 0);
				exported virtual bool send(const uint8* buffer, uint16 length);

				/**
				 * Add @a buffer to send queue
				 */
				exported void addPart(const uint8* buffer, uint16 length);

				/**
				 * Send all buffers queued with @a addPart
				 */
				exported virtual bool sendParts();

			protected:
				static const uint32 MESSAGE_LENGTH_BYTES = 2;
				static const uint32 MESSAGE_MAX_SIZE = 65536 + MESSAGE_LENGTH_BYTES;
			
				Socket connection;
				TCPServer* owningServer; //null when this client is used to connect to a remote server
				uint8 buffer[MESSAGE_MAX_SIZE];
				uint16 bytesReceived;
				void* state;
				bool connected;
				std::vector< std::pair<const uint8*, uint16> > messageParts;
				
				TCPConnection(TCPServer* server, Socket& socket);
				virtual void disconnect(bool callServerDisconnect = true);

			private:
				TCPConnection(const TCPConnection& other);
				TCPConnection& operator=(const TCPConnection& other);

				friend class TCPServer;
		};
	}
}
