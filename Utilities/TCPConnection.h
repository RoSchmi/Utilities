#pragma once

#include "Common.h"
#include "Socket.h"

#include <vector>
#include <array>

namespace Utilities {
	namespace Net {
		///An abstraction over a socket. Uses minimal framing with two leading length bytes.
		class TCPConnection {
			public:
				///The number of bytes used to determine the message length.
				static const word MESSAGE_LENGTH_BYTES = 2;

				///The maximum length a message may be including the leading length bytes.
				static const word MESSAGE_MAX_SIZE = 0xFFFF + MESSAGE_LENGTH_BYTES;

				///Represents a message that is generated when reading from the connection.
				struct exported Message {
					///The length of the message excluding the length bytes themselves.
					word length;

					///The actual message data excluding the length bytes themselves.
					uint8* data;

					///A flag signaling that the connection was closed.
					bool wasClosed;

					///Copies an existing message into this message.
					///@param other The message copied from. 
					///@return This message.
					Message& operator=(const Message& other);

					///Moves an existing message into this message.
					///@param other The message to move. 
					///@return This message.
					Message& operator=(Message&& other);

					///Constructs this message by moving from another message.
					///@param other The message to move from. 
					///@return This message.
					Message(Message&& other);

					///Constructs a new message with no data and the given closed flag.
					///@param closed Whether or not the connection was closed. 
					Message(bool closed);

					///Constructs a new message using the given buffer and length.
					///@param buffer The received data. 
					///@param length The number of bytes received. 
					Message(const uint8* buffer, word length);

					///Constructs this message by copying from another message.
					///@param other The message to copy from. 
					Message(const Message& other);

					///Destructs the instance.
					~Message();
				};

				///State to be stored with this connection.
				///Not used in any way by this class
				void* state;

				///Constructs an unconnected instance.
				///You must move assign to make use of it.
				exported TCPConnection();
				
				///Constructs a new TCPConnection by using an existing socket.
				///Takes ownership of the socket.
				exported TCPConnection(Socket&& socket);

				///Constructs a new TCPConnection by establishing a new connection to the specified address and port.
				///@param address The address to connect to. 
				///@param port The port to connect to. 
				///@param state The state to store in this instance. 
				exported TCPConnection(std::string address, std::string port, void* state = nullptr);

				///Constructs this connection by moving from another connection.
				///@param other The message to move from. 
				exported TCPConnection(TCPConnection&& other);

				///Moves an existing connection into this connection.
				///@param other The connection to move. 
				///@return This connection.
				exported TCPConnection& operator=(TCPConnection&& other);

				///Gets the IP address of the underlying socket.
				///@return The IP address.
				exported std::array<uint8, Socket::ADDRESS_LENGTH> getAddress() const;

				///Gets the underlying socket.
				///@return The socket.
				exported const Socket& getBaseSocket() const;

				///Gets whether or not data is available to be read.
				///@return True if data is available, false otherwise.
				exported bool isDataAvailable() const;

				///Gets a list of messages that are available and complete.
				///@param messagesToWaitFor The number of messages to wait for. Defaults to zero. 
				///@return A vector of possible zero messages that were read.
				exported virtual std::vector<Message> read(word messagesToWaitFor = 0);

				///Sends the given data over the connection.
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				///@return True if all the data was sent, false otherwise.
				exported virtual bool send(const uint8* buffer, word length);

				///Adds the data to the internal pending queue.
				///Call sendParts to send all the data queued with this message as one contiguous message
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				exported void addPart(const uint8* buffer, word length);

				///Sends all the data queued with addPart as one contiguous message.
				///@return True if all the data was sent, false otherwise.
				exported virtual bool sendParts();

				///Clears without sending the data in the internal pending queue.
				exported void clearParts();

				///Closes the underlying connection.
				exported virtual void close();

				///Destructs the instance.
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
