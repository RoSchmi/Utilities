#include "Socket.h"

#include <utility>
#include <memory>
#include <algorithm>
#include <cstring>

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define FD_SETSIZE 1024
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <Windows.h>

	static bool winsock_initialized = false;
#elif defined POSIX
	#include <sys/select.h>
	#include <sys/Socket.h>
	#include <sys/types.h>
	#include <netinet/in.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <stdio.h>
	#include <string.h>
	#include <endian.h>
#endif

using namespace std;
using namespace util;
using namespace util::net;

#ifdef WINDOWS
uintptr prep_socket(socket::families family, socket::types type, string address, string port, bool is_listener, addrinfo** addr_info) {
#elif defined POSIX
int prep_socket(socket::families family, socket::types type, string address, string port, bool is_listener, addrinfo** addr_info) {
#endif
	addrinfo hints;
	addrinfo* server_addr_info;

#ifdef WINDOWS
	if (!::winsock_initialized) {
		WSADATA startup_data;
		if (::WSAStartup(514, &startup_data) != 0)
			throw runtime_error("WinSock failed to initialize.");
		::winsock_initialized = true;
	}
	uintptr raw_socket;
#elif defined POSIX
	int raw_socket;
#endif

	memset(&hints, 0, sizeof(addrinfo));

	switch (family) {
		case socket::families::IPV4: hints.ai_family = AF_INET; break;
		case socket::families::IPV6: hints.ai_family = AF_INET6; break;
		#ifdef WINDOWS
		case socket::families::IPAny: hints.ai_family = AF_INET6; break;
		#elif defined POSIX
		case socket::families::IPAny: hints.ai_family = AF_UNSPEC; break;
		#endif
	}

	switch (type) {
		case socket::types::TCP: hints.ai_socktype = SOCK_STREAM; break;
	}

	if (is_listener)
		hints.ai_flags = AI_PASSIVE;
		
	if (::getaddrinfo(address != "" ? address.c_str() : nullptr, port.c_str(), &hints, &server_addr_info) != 0 || server_addr_info == nullptr)
		throw socket::invalid_address_exception();

	raw_socket = ::socket(server_addr_info->ai_family, server_addr_info->ai_socktype, server_addr_info->ai_protocol);
#ifdef WINDOWS
	if (raw_socket == INVALID_SOCKET) {
		::closesocket(raw_socket);
		::freeaddrinfo(server_addr_info);
		throw socket::could_not_create_exception();
	}
	else {
		int opt = 0;
		::setsockopt(raw_socket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&opt), sizeof(opt));
	}
#elif defined POSIX
	if (raw_socket == -1) {
		::close(raw_socket);
		::freeaddrinfo(server_addr_info);
		throw socket::could_not_create_exception();
	}
#endif

	*addr_info = server_addr_info;

	return raw_socket;
}

socket::socket(families family, types type) {
	this->type = type;
	this->family = family;
	this->connected = false;
	this->endpoint_address.fill(0x00);

	#ifdef WINDOWS
		this->raw_socket = INVALID_SOCKET;
	#elif defined POSIX
		this->raw_socket = -1;
	#endif
}

socket::socket() {
	this->connected = false;
}

socket::socket(families family, types type, string address, string port) : socket(family, type) {
	addrinfo* server_addr_info;
	
	this->raw_socket = prep_socket(family, type, address, port, false, &server_addr_info);
	
	if (::connect(this->raw_socket, server_addr_info->ai_addr, static_cast<int>(server_addr_info->ai_addrlen)) != 0) {
		#ifdef WINDOWS
			::closesocket(this->raw_socket);
		#elif defined POSIX
			::close(this->raw_socket);
		#endif

		::freeaddrinfo(server_addr_info);
		
		throw could_not_connect_exception();
	}

	this->connected = true;
	
	::freeaddrinfo(server_addr_info);
}

socket::socket(families family, types type, string port) : socket(family, type) {
	addrinfo* server_addr_info;

	this->raw_socket = prep_socket(family, type, "", port, true, &server_addr_info);

	if (::bind(this->raw_socket, server_addr_info->ai_addr, (int)server_addr_info->ai_addrlen) != 0)
		goto error;

	if (::listen(this->raw_socket, SOMAXCONN) != 0)
		goto error;
	
	::freeaddrinfo(server_addr_info);

	return;

error:
#ifdef WINDOWS
	::closesocket(this->raw_socket);
#elif defined POSIX
	::close(this->raw_socket);
#endif

	freeaddrinfo(server_addr_info);
	
	throw could_not_listen_exception();
}

socket::socket(socket&& other) {
	this->connected = false;
	*this = move(other);
}

socket::~socket() {
	this->close();
}

net::socket& socket::operator=(socket&& other) {
	if (this->connected)
		this->close();
		
	this->type = other.type;
	this->family = other.family;

	this->connected = other.connected;
	other.connected = false;

	this->endpoint_address = other.endpoint_address;

	this->raw_socket = other.raw_socket;
#ifdef WINDOWS
	other.raw_socket = INVALID_SOCKET;
#elif defined POSIX
	other.raw_socket = -1;
#endif

	return *this;
}

void socket::close() {
	if (!this->connected)
		return;

	this->connected = false;

#ifdef WINDOWS
	::shutdown(this->raw_socket, SD_BOTH);
	::closesocket(this->raw_socket);
	this->raw_socket = INVALID_SOCKET;
#elif defined POSIX
	::shutdown(this->raw_socket, SHUT_RDWR);
	::close(this->raw_socket);
	this->raw_socket = -1;
#endif
}

net::socket socket::accept() {
	if (!this->connected)
		throw not_connected_exception();

	socket new_socket(this->family, this->type);
	sockaddr_storage remote_address;
	size_t address_length = sizeof(remote_address);

#ifdef WINDOWS
	new_socket.raw_socket = ::accept(this->raw_socket, reinterpret_cast<sockaddr*>(&remote_address), reinterpret_cast<int*>(&address_length));
	if (new_socket.raw_socket == INVALID_SOCKET)
		return new_socket;
#elif defined POSIX
	new_socket.raw_socket = ::accept(this->raw_socket, reinterpret_cast<sockaddr* > (&remote_address), reinterpret_cast<socklen_t*>(&address_length));
	if (new_socket.raw_socket == -1)
		return new_socket;
#endif
	
	new_socket.connected = true;

	if (remote_address.ss_family == AF_INET) {
		sockaddr_in* ipv4 = reinterpret_cast<sockaddr_in*>(&remote_address);
		memset(new_socket.endpoint_address.data(), 0, 10); //to copy the ipv4 address in ipv6 mapped format
		memset(new_socket.endpoint_address.data() + 10, 1, 2);
		#ifdef WINDOWS
		memcpy(new_socket.endpoint_address.data() + 12, reinterpret_cast<uint8*>(&ipv4->sin_addr.S_un.S_addr), 4);
		#elif defined POSIX
		memcpy(new_socket.endpoint_address.data() + 12, reinterpret_cast<uint8*>(&ipv4->sin_addr.s_addr), 4);
		#endif
	}
	else if (remote_address.ss_family == AF_INET6) {
		sockaddr_in6* ipv6 = reinterpret_cast<sockaddr_in6*>(&remote_address);
		#ifdef WINDOWS
		memcpy(new_socket.endpoint_address.data(), ipv6->sin6_addr.u.Byte, sizeof(ipv6->sin6_addr.u.Byte));
		#elif defined POSIX
		memcpy(new_socket.endpoint_address.data(), ipv6->sin6_addr.s6_addr, sizeof(ipv6->sin6_addr.s6_addr));
		#endif
	}

	return new_socket;
}

word socket::read(uint8* buffer, word count) {
	if (!this->connected)
		throw not_connected_exception();

	int received = ::recv(this->raw_socket, reinterpret_cast<char*>(buffer), static_cast<int>(count), 0);
	if (received <= 0)
		return 0;

	return static_cast<word>(received);
}

word socket::write(const uint8* buffer, word count) {
	if (!this->connected)
		throw not_connected_exception();

	if (count == 0)
		return 0;

	return static_cast<word>(::send(this->raw_socket, reinterpret_cast<const char*>(buffer), static_cast<int>(count), 0));
}

array<uint8, socket::ADDRESS_LENGTH> socket::remote_address() const {
	if (!this->connected)
		throw not_connected_exception();

	return this->endpoint_address;
}

bool socket::is_connected() const {
	return this->connected;
}

bool socket::data_available() const {
	if (!this->connected)
		throw not_connected_exception();

	static fd_set read_set;
	FD_ZERO(&read_set);

	timeval timeout;
	timeout.tv_usec = 250;
	timeout.tv_sec = 0;
	
	FD_SET(this->raw_socket, &read_set);
	
#ifdef WINDOWS
	return ::select(0, &read_set, nullptr, nullptr, &timeout) > 0;
#elif defined POSIX
	return ::select(this->raw_socket + 1, &read_set, nullptr, nullptr, &timeout) > 0;
#endif
}


int16 util::net::host_to_net_int16(int16 value) {
	return htons(value);
}

int32 util::net::host_to_net_int32(int32 value) {
	return htonl(value);
}

int64 util::net::host_to_net_int64(int64 value) {
#ifdef WINDOWS
	return htonll(value);
#elif defined POSIX
	return htobe64(value); // XXX: glibc only
#endif
}

int16 util::net::net_to_host_int16(int16 value) {
	return ntohs(value);
}

int32 util::net::net_to_host_int32(int32 value) {
	return ntohl(value);
}

int64 util::net::net_to_host_int64(int64 value) {
#ifdef WINDOWS
	return ntohll(value);
#elif defined POSIX
	return be64toh(value); // XXX: glibc only
#endif
}