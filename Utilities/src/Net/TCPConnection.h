#pragma once

#include <vector>
#include <array>

#include "../Common.h"
#include "Socket.h"

namespace util {
	namespace net {
		///An abstraction over a socket. Uses minimal framing with two leading length bytes.
		class tcp_connection {
			public:
				///The number of bytes used to determine the message length.
				static const word message_length_bytes = 2;

				///The maximum length a message may be including the leading length bytes.
				static const word message_max_size = 0xFFFF + message_length_bytes;

				///Represents a message that is generated when reading from the connection.
				struct message {
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
				tcp_connection();
				
				///Constructs a new tcp_connection by using an existing socket.
				///Takes ownership of the socket.
				tcp_connection(socket&& sock);

				///Constructs a new tcp_connection by establishing a new connection to the specified address and port.
				///@param address The address to connect to. 
				///@param port The port to connect to. 
				tcp_connection(endpoint ep);

				///Constructs this connection by moving from another connection.
				///@param other The message to move from. 
				tcp_connection(tcp_connection&& other);

				///Moves an existing connection into this connection.
				///@param other The connection to move. 
				///@return This connection.
				tcp_connection& operator=(tcp_connection&& other);

				///Gets the IP address of the underlying socket.
				///@return The IP address.
				std::array<uint8, socket::address_length> address() const;

				///Gets the underlying socket.
				///@return The socket.
				const socket& base_socket() const;

				///@return Whether or not the connection is open.
				bool connected() const;

				///Gets whether or not data is available to be read.
				///@return True if data is available, false otherwise.
				bool data_available() const;

				///Gets a list of messages that are available and complete.
				///@param wait_for The number of messages to wait for. Defaults to zero. 
				///@return A vector of possible zero messages that were read.
				virtual std::vector<message> read(word wait_for = 0);

				///Sends the given data over the connection.
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				///@return True if all the data was sent, false otherwise.
				virtual bool send(const uint8* buffer, word length);

				///Adds the data to the internal pending queue.
				///Call send_queued to send all the data queued with this message as one contiguous message
				///@param buffer The data to send. 
				///@param length The number of bytes to be sent. 
				void enqueue(const uint8* buffer, word length);

				///Sends all the data queued with enqueue as one contiguous message.
				///@return True if all the data was sent, false otherwise.
				virtual bool send_queued();

				///Clears without sending the data in the internal pending queue.
				void clear_queued();

				///Closes the underlying connection.
				virtual void close();

				///Destructs the instance.
				virtual ~tcp_connection();

#ifdef WINDOWS
				//Hack to work around the fact that std::bind inappropraitely binds to rvalues.
				//http://connect.microsoft.com/VisualStudio/feedback/details/717188/bug-concerning-rvalue-references-and-std-function-std-bind-in-vc11
				//The copy constructor actually moves from the other object and uses a const_cast.
				//TCPServer on_connect also passes by value instead of by move, change that back to move when this is fixed.
				tcp_connection(const tcp_connection& other);
#elif defined POSIX
				tcp_connection(const tcp_connection& other) = delete;
#endif

				tcp_connection& operator=(const tcp_connection& other) = delete;

			protected:
				socket connection;
				uint8* buffer;
				word received;
				std::vector<message> queued;

				bool ensure_write(const uint8* data, word count);
		};
	}
}
