#pragma once

#include "../Common.h"
#include "Socket.h"

#include <vector>
#include <array>

namespace util {
	namespace net {
		///An abstraction over a socket. Uses minimal framing with two leading length bytes.
		class tcp_connection {
			public:
				///The number of bytes used to determine the message length.
				static const word MESSAGE_LENGTH_BYTES = 2;

				///The maximum length a message may be including the leading length bytes.
				static const word MESSAGE_MAX_SIZE = 0xFFFF + MESSAGE_LENGTH_BYTES;

				///Represents a message that is generated when reading from the connection.
				struct exported message {
					///The length of the message excluding the length bytes themselves.
					word length;

					///The actual message data excluding the length bytes themselves.
					uint8* data;

					///A flag signaling that the connection was closed.
					bool closed;

					///Copies an existing message into this message.
					///@param other The message copied from. 
					///@return This message.
					message& operator=(const message& other);

					///Moves an existing message into this message.
					///@param other The message to move. 
					///@return This message.
					message& operator=(message&& other);

					///Constructs this message by moving from another message.
					///@param other The message to move from. 
					///@return This message.
					message(message&& other);

					///Constructs a new message with no data and the given closed flag.
					///@param closed Whether or not the connection was closed. 
					message(bool closed);

					///Constructs a new message using the given buffer and length.
					///@param buffer The received data. 
					///@param length The number of bytes received. 
					message(const uint8* buffer, word length);

					///Constructs this message by copying from another message.
					///@param other The message to copy from. 
					message(const message& other);

					///Destructs the instance.
					~message();
				};

				class not_connected_exception {};
				class message_too_long_exception {};

				///State to be stored with this connection.
				///Not used in any way by this class
				void* state;

				///Constructs an unconnected instance.
				///You must move assign to make use of it.
				exported tcp_connection();
				
				///Constructs a new tcp_connection by using an existing socket.
				///Takes ownership of the socket.
				exported tcp_connection(socket&& sock);

				///Constructs a new tcp_connection by establishing a new connection to the specified address and port.
				///@param address The address to connect to. 
				///@param port The port to connect to. 
				///@param state The state to store in this instance. 
				exported tcp_connection(std::string address, std::string port, void* state = nullptr);

				///Constructs this connection by moving from another connection.
				///@param other The message to move from. 
				exported tcp_connection(tcp_connection&& other);

				///Moves an existing connection into this connection.
				///@param other The connection to move. 
				///@return This connection.
				exported tcp_connection& operator=(tcp_connection&& other);

				///Gets the IP address of the underlying socket.
				///@return The IP address.
				exported std::array<uint8, socket::ADDRESS_LENGTH> address() const;

				///Gets the underlying socket.
				///@return The socket.
				exported const socket& base_socket() const;

				///Gets whether or not data is available to be read.
				///@return True if data is available, false otherwise.
				exported bool data_available() const;

				///Gets a list of messages that are available and complete.
				///@param wait_for The number of messages to wait for. Defaults to zero. 
				///@return A vector of possible zero messages that were read.
				exported virtual std::vector<message> read(word wait_for = 0);

				///Sends the given data over the connection.
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				///@return True if all the data was sent, false otherwise.
				exported virtual bool send(const uint8* buffer, word length);

				///Adds the data to the internal pending queue.
				///Call send_queued to send all the data queued with this message as one contiguous message
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				exported void enqueue(const uint8* buffer, word length);

				///Sends all the data queued with enqueue as one contiguous message.
				///@return True if all the data was sent, false otherwise.
				exported virtual bool send_queued();

				///Clears without sending the data in the internal pending queue.
				exported void clear_queued();

				///Closes the underlying connection.
				exported virtual void close();

				///Destructs the instance.
				exported virtual ~tcp_connection();

				tcp_connection(const tcp_connection& other) = delete;
				tcp_connection& operator=(const tcp_connection& other) = delete;

			protected:
				socket connection;
				uint8* buffer;
				word received;
				bool connected;
				std::vector<message> queued;

				bool ensure_write(const uint8* toWrite, word writeAmount);
		};
	}
}
