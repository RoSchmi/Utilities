#pragma once

#include "Common.h"
#include "Socket.h"

#include <vector>
#include <array>

namespace Utilities {
	namespace Net {
		///<summary>An abstraction over a socket. Uses minimal framing with two leading length bytes.</summary>
		class TCPConnection {
			public:
				///<summary>The number of bytes used to determine the message length.</summary>
				static const word MESSAGE_LENGTH_BYTES = 2;

				///<summary>The maximum length a message may be including the leading length bytes.</summary>
				static const word MESSAGE_MAX_SIZE = 0xFFFF + MESSAGE_LENGTH_BYTES;

				///<summary>Represents a message that is generated when reading from the connection.</summary>
				struct exported Message {
					///<summary>The length of the message excluding the length bytes themselves.</summary>
					word length;

					///<summary>The actual message data excluding the length bytes themselves.</summary>
					uint8* data;

					///<summary>A flag signaling that the connection was closed.</summary>
					bool wasClosed;

					///<summary>Copies an existing message into this message.</summary>
					///<param name="other">The message copied from.</param>
					///<returns>This message.</returns>
					Message& operator=(const Message& other);

					///<summary>Moves an existing message into this message.</summary>
					///<param name="other">The message to move.</param>
					///<returns>This message.</returns>
					Message& operator=(Message&& other);

					///<summary>Constructs this message by moving from another message.</summary>
					///<param name="other">The message to move from.</param>
					///<returns>This message.</returns>
					Message(Message&& other);


					///<summary>Constructs a new message with no data and the given closed flag.</summary>
					///<param name="closed">Whether or not the connection was closed.</param>
					Message(bool closed);

					///<summary>Constructs a new message using the given buffer and length.</summary>
					///<param name="buffer">The received data.</param>
					///<param name="length">The number of bytes received.</param>
					Message(const uint8* buffer, word length);

					///<summary>Constructs this message by copying from another message.</summary>
					///<param name="other">The message to copy from.</param>
					Message(const Message& other);

					~Message();

					Message() = delete;

					private:

						friend class TCPConnection;
						friend class WebSocketConnection;
				};

				///<summary>State to be stored with this connection.</summary>
				///<remarks>Not used in any way by this class</remarks>
				void* state;
				
				///<summary>Constructs a new TCPConnection by using an existing socket.</summary>
				///<remarks>Takes ownership of the socket.</remarks>
				exported TCPConnection(Socket&& socket);

				///<summary>Constructs a new TCPConnection by establishing a new connection to the specified address and port.</summary>
				///<param name="address">The address to connect to.</param>
				///<param name="port">The port to connect to.</param>
				///<param name="state">The state to store in this instance.</param>
				exported TCPConnection(std::string address, std::string port, void* state = nullptr);

				///<summary>Constructs this connection by moving from another connection.</summary>
				///<param name="other">The message to move from.</param>
				exported TCPConnection(TCPConnection&& other);

				///<summary>Moves an existing connection into this connection.</summary>
				///<param name="other">The connection to move.</param>
				///<returns>This connection.</returns>
				exported TCPConnection& operator=(TCPConnection&& other);

				///<summary>Gets the IP address of the underlying socket.</summary>
				///<returns>The IP address.</returns>
				exported std::array<uint8, Socket::ADDRESS_LENGTH> getAddress() const;

				///<summary>Gets the underlying socket.</summary>
				///<returns>The socket.</returns>
				exported const Socket& getBaseSocket() const;

				///<summary>Gets whether or not data is available to be read.</summary>
				///<returns>True if data is available, false otherwise.</returns>
				exported bool isDataAvailable() const;

				///<summary>Gets a list of messages that are available and complete.</summary>
				///<param name="messagesToWaitFor">The number of messages to wait for. Defaults to zero.</param>
				///<returns>A vector of possible zero messages that were read.</returns>
				exported virtual std::vector<const Message> read(word messagesToWaitFor = 0);

				///<summary>Sends the given data over the connection.</summary>
				///<param name="buffer">The data to send.</param>
				///<param name="length">The number of bytes to be sent.</param>
				///<returns>True if all the data was sent, false otherwise.</returns>
				exported virtual bool send(const uint8* buffer, word length);

				///<summary>Adds the data to the internal pending queue.</summary>
				///<remarks>Call <see cref="sendParts" /> to send all the data queued with this message as one contiguous message</remarks>
				///<param name="buffer">The data to send.</param>
				///<param name="length">The number of bytes to be sent.</param>
				exported void addPart(const uint8* buffer, word length);

				///<summary>Sends all the data queued with <see cref="addPart" /> as one contiguous message.</summary>
				///<returns>True if all the data was sent, false otherwise.</returns>
				exported virtual bool sendParts();

				///<summary>Closes the underlying connection.</summary>
				exported virtual void close();

				exported virtual ~TCPConnection();

				TCPConnection(const TCPConnection& other) = delete;
				TCPConnection& operator=(const TCPConnection& other) = delete;

			protected:
				Socket connection;
				uint8* buffer;
				word bytesReceived;
				bool connected;
				std::vector<Message> messageParts;

				bool ensureWrite(const uint8* toWrite, word writeAmount);
		};
	}
}
