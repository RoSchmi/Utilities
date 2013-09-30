#include "Socket.h"

#include <utility>
#include <stdexcept>
#include <memory>
#include <algorithm>
#include <cstring>

#ifdef WINDOWS
	#define WIN32_LEAN_AND_MEAN
	#define FD_SETSIZE 1024
	#include <winsock2.h>
	#include <ws2tcpip.h>
	#include <Windows.h>

	static bool winsockInitialized = false;
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
uintptr prepareRawSocket(socket::families family, socket::types type, string address, string port, bool isListener, addrinfo** addressInfo) {
#elif defined POSIX
int prepareRawSocket(socket::families family, socket::types type, string address, string port, bool isListener, addrinfo** addressInfo) {
#endif
	addrinfo serverHints;
	addrinfo* serverAddrInfo;

#ifdef WINDOWS
	if (!::winsockInitialized) {
		WSADATA startupData;
		if (::WSAStartup(514, &startupData) != 0)
			throw runtime_error("WinSock failed to initialize.");
		::winsockInitialized = true;
	}
	uintptr rawSocket;
#elif defined POSIX
	int rawSocket;
#endif

	memset(&serverHints, 0, sizeof(addrinfo));

	switch (family) {
		case socket::families::IPV4: serverHints.ai_family = AF_INET; break;
		case socket::families::IPV6: serverHints.ai_family = AF_INET6; break;
		#ifdef WINDOWS
		case socket::families::IPAny: serverHints.ai_family = AF_INET6; break;
		#elif defined POSIX
		case socket::families::IPAny: serverHints.ai_family = AF_UNSPEC; break;
		#endif
		default: throw runtime_error("Invalid socket Family.");
	}

	switch (type) {
		case socket::types::TCP: serverHints.ai_socktype = SOCK_STREAM; break;
		default: throw runtime_error("Invalid socket Type.");
	}

	if (isListener)
		serverHints.ai_flags = AI_PASSIVE;
		
	if (::getaddrinfo(address != "" ? address.c_str() : nullptr, port.c_str(), &serverHints, &serverAddrInfo) != 0 || serverAddrInfo == nullptr)
		throw runtime_error("Unable to get socket address information.");

	rawSocket = ::socket(serverAddrInfo->ai_family, serverAddrInfo->ai_socktype, serverAddrInfo->ai_protocol);
#ifdef WINDOWS
	if (rawSocket == INVALID_SOCKET) {
		::closesocket(rawSocket);
		::freeaddrinfo(serverAddrInfo);
		throw runtime_error("Unable to create socket.");
	}
	else {
		int opt = 0;
		::setsockopt(rawSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&opt), sizeof(opt));
	}
#elif defined POSIX
	if (rawSocket == -1) {
		::close(rawSocket);
		::freeaddrinfo(serverAddrInfo);
		throw runtime_error("Unable to create socket.");
	}
#endif

	*addressInfo = serverAddrInfo;

	return rawSocket;
}

socket::socket(families family, types type) {
	this->type = type;
	this->family = family;
	this->connected = false;
	this->remoteEndpointAddress.fill(0x00);

	#ifdef WINDOWS
		this->rawSocket = INVALID_SOCKET;
	#elif defined POSIX
		this->rawSocket = -1;
	#endif
}

socket::socket() {
	this->connected = false;
}

socket::socket(families family, types type, string address, string port) : socket(family, type) {
	addrinfo* serverAddrInfo;
	
	this->rawSocket = prepareRawSocket(family, type, address, port, false, &serverAddrInfo);
	
	if (::connect(this->rawSocket, serverAddrInfo->ai_addr, static_cast<int>(serverAddrInfo->ai_addrlen)) != 0) {
		#ifdef WINDOWS
			::closesocket(this->rawSocket);
		#elif defined POSIX
			::close(this->rawSocket);
		#endif

		::freeaddrinfo(serverAddrInfo);
		
		throw runtime_error("Couldn't connect to server.");
	}

	this->connected = true;
	
	::freeaddrinfo(serverAddrInfo);
}

socket::socket(families family, types type, string port) : socket(family, type) {
	addrinfo* serverAddrInfo;

	this->rawSocket = prepareRawSocket(family, type, "", port, true, &serverAddrInfo);

	if (::bind(this->rawSocket, serverAddrInfo->ai_addr, (int)serverAddrInfo->ai_addrlen) != 0)
		goto error;

	if (::listen(this->rawSocket, SOMAXCONN) != 0)
		goto error;
	
	::freeaddrinfo(serverAddrInfo);

	return;

error:
#ifdef WINDOWS
	::closesocket(this->rawSocket);
#elif defined POSIX
	::close(this->rawSocket);
#endif

	freeaddrinfo(serverAddrInfo);
	
	throw runtime_error("Couldn't listen on port.");
}

socket::socket(socket&& other) {
	this->connected = false;
	*this = std::move(other);
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

	this->remoteEndpointAddress = other.remoteEndpointAddress;

	this->rawSocket = other.rawSocket;
#ifdef WINDOWS
	other.rawSocket = INVALID_SOCKET;
#elif defined POSIX
	other.rawSocket = -1;
#endif

	return *this;
}

void socket::close() {
	if (!this->connected)
		return;

	this->connected = false;

#ifdef WINDOWS
	::shutdown(this->rawSocket, SD_BOTH);
	::closesocket(this->rawSocket);
	this->rawSocket = INVALID_SOCKET;
#elif defined POSIX
	::shutdown(this->rawSocket, SHUT_RDWR);
	::close(this->rawSocket);
	this->rawSocket = -1;
#endif
}

net::socket socket::accept() {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	socket newSocket(this->family, this->type);
	sockaddr_storage remoteAddress;
	size_t addressLength = sizeof(remoteAddress);

#ifdef WINDOWS
	newSocket.rawSocket = ::accept(this->rawSocket, reinterpret_cast<sockaddr*>(&remoteAddress), reinterpret_cast<int*>(&addressLength));
	if (newSocket.rawSocket == INVALID_SOCKET)
		return newSocket;
#elif defined POSIX
	newSocket.rawSocket = ::accept(this->rawSocket, reinterpret_cast<sockaddr* > (&remoteAddress), reinterpret_cast<socklen_t*>(&addressLength));
	if (newSocket.rawSocket == -1)
		return newSocket;
#endif
	
	newSocket.connected = true;

	if (remoteAddress.ss_family == AF_INET) {
		sockaddr_in* ipv4Address = reinterpret_cast<sockaddr_in*>(&remoteAddress);
		memset(newSocket.remoteEndpointAddress.data(), 0, 10); //to copy the ipv4 address in ipv6 mapped format
		memset(newSocket.remoteEndpointAddress.data() + 10, 1, 2);
		#ifdef WINDOWS
		memcpy(newSocket.remoteEndpointAddress.data() + 12, reinterpret_cast<uint8*>(&ipv4Address->sin_addr.S_un.S_addr), 4);
		#elif defined POSIX
		memcpy(newSocket.remoteEndpointAddress.data() + 12, reinterpret_cast<uint8*>(&ipv4Address->sin_addr.s_addr), 4);
		#endif
	}
	else if (remoteAddress.ss_family == AF_INET6) {
		sockaddr_in6* ipv6Address = reinterpret_cast<sockaddr_in6*>(&remoteAddress);
		#ifdef WINDOWS
		memcpy(newSocket.remoteEndpointAddress.data(), ipv6Address->sin6_addr.u.Byte, sizeof(ipv6Address->sin6_addr.u.Byte));
		#elif defined POSIX
		memcpy(newSocket.remoteEndpointAddress.data(), ipv6Address->sin6_addr.s6_addr, sizeof(ipv6Address->sin6_addr.s6_addr));
		#endif
	}

	return newSocket;
}

word socket::read(uint8* buffer, word bufferSize) {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	int received = ::recv(this->rawSocket, reinterpret_cast<char*>(buffer), static_cast<int>(bufferSize), 0);
	if (received <= 0)
		return 0;

	return static_cast<word>(received);
}

word socket::write(const uint8* toWrite, word writeAmount) {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	if (writeAmount == 0)
		return 0;

	return static_cast<word>(::send(this->rawSocket, reinterpret_cast<const char*>(toWrite), static_cast<int>(writeAmount), 0));
}

array<uint8, socket::ADDRESS_LENGTH> socket::getRemoteAddress() const {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	return this->remoteEndpointAddress;
}

bool socket::isConnected() const {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	return this->connected;
}

bool socket::isDataAvailable() const {
	if (!this->connected)
		throw runtime_error("socket is not open.");

	static fd_set readSet;
	FD_ZERO(&readSet);

	timeval timeout;
	timeout.tv_usec = 250;
	timeout.tv_sec = 0;
	
	FD_SET(this->rawSocket, &readSet);
	
#ifdef WINDOWS
	return ::select(0, &readSet, nullptr, nullptr, &timeout) > 0;
#elif defined POSIX
	return ::select(this->rawSocket + 1, &readSet, nullptr, nullptr, &timeout) > 0;
#endif
}


int16 util::net::hostToNetworkInt16(int16 value) {
	return htons(value);
}

int32 util::net::hostToNetworkInt32(int32 value) {
	return htonl(value);
}

int64 util::net::hostToNetworkInt64(int64 value) {
#ifdef WINDOWS
	return htonll(value);
#elif defined POSIX
	return htobe64(value); // XXX: glibc only
#endif
}

int16 util::net::networkToHostInt16(int16 value) {
	return ntohs(value);
}

int32 util::net::networkToHostInt32(int32 value) {
	return ntohl(value);
}

int64 util::net::networkToHostInt64(int64 value) {
#ifdef WINDOWS
	return ntohll(value);
#elif defined POSIX
	return be64toh(value); // XXX: glibc only
#endif
}