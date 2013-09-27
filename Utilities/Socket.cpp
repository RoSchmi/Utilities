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
	#include <sys/socket.h>
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
using namespace Utilities;
using namespace Utilities::Net;

#ifdef WINDOWS
uintptr prepareRawSocket(Socket::Families family, Socket::Types type, string address, string port, bool isListener, addrinfo** addressInfo) {
#elif defined POSIX
int prepareRawSocket(Socket::Families family, Socket::Types type, string address, string port, bool isListener, addrinfo** addressInfo) {
#endif
	addrinfo serverHints;
	addrinfo* serverAddrInfo;

#ifdef WINDOWS
	if (!::winsockInitialized) {
		WSADATA startupData;
		::WSAStartup(514, &startupData);
		::winsockInitialized = true;
	}
	uintptr rawSocket;
#elif defined POSIX
	int rawSocket;
#endif

	memset(&serverHints, 0, sizeof(addrinfo));

	switch (family) {
		case Socket::Families::IPV4: serverHints.ai_family = AF_INET; break;
		case Socket::Families::IPV6: serverHints.ai_family = AF_INET6; break;
		#ifdef WINDOWS
		case Socket::Families::IPAny: serverHints.ai_family = AF_INET6; break;
		#elif defined POSIX
		case Socket::Families::IPAny: serverHints.ai_family = AF_UNSPEC; break;
		#endif
		default: throw runtime_error("Invalid Socket Family.");
	}

	switch (type) {
		case Socket::Types::TCP: serverHints.ai_socktype = SOCK_STREAM; break;
		default: throw runtime_error("Invalid Socket Type.");
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

Socket::Socket(Families family, Types type) {
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

Socket::Socket(Families family, Types type, string address, string port) : Socket(family, type) {
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

Socket::Socket(Families family, Types type, string port) : Socket(family, type) {
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

Socket::Socket(Socket&& other) {
	this->connected = false;
	*this = std::move(other);
}

Socket::~Socket() {
	this->close();
}

Socket& Socket::operator=(Socket&& other) {
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

void Socket::close() {
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

Socket Socket::accept() {
	if (!this->connected)
		throw runtime_error("Socket was closed.");

	Socket socket(this->family, this->type);
	sockaddr_storage remoteAddress;
	size_t addressLength = sizeof(remoteAddress);

#ifdef WINDOWS
	socket.rawSocket = ::accept(this->rawSocket, reinterpret_cast<sockaddr*>(&remoteAddress), reinterpret_cast<int*>(&addressLength));
	if (socket.rawSocket == INVALID_SOCKET)
		return socket;
#elif defined POSIX
	socket.rawSocket = ::accept(this->rawSocket, reinterpret_cast<sockaddr* > (&remoteAddress), reinterpret_cast<socklen_t*>(&addressLength));
	if (socket.rawSocket == -1)
		return socket;
#endif
	
	socket.connected = true;

	if (remoteAddress.ss_family == AF_INET) {
		sockaddr_in* ipv4Address = reinterpret_cast<sockaddr_in*>(&remoteAddress);
		memset(socket.remoteEndpointAddress.data(), 0, 10); //to copy the ipv4 address in ipv6 mapped format
		memset(socket.remoteEndpointAddress.data() + 10, 1, 2);
		#ifdef WINDOWS
		memcpy(socket.remoteEndpointAddress.data() + 12, reinterpret_cast<uint8*>(&ipv4Address->sin_addr.S_un.S_addr), 4);
		#elif defined POSIX
		memcpy(socket.remoteEndpointAddress.data() + 12, reinterpret_cast<uint8*>(&ipv4Address->sin_addr.s_addr), 4);
		#endif
	}
	else if (remoteAddress.ss_family == AF_INET6) {
		sockaddr_in6* ipv6Address = reinterpret_cast<sockaddr_in6*>(&remoteAddress);
		#ifdef WINDOWS
		memcpy(socket.remoteEndpointAddress.data(), ipv6Address->sin6_addr.u.Byte, sizeof(ipv6Address->sin6_addr.u.Byte));
		#elif defined POSIX
		memcpy(socket.remoteEndpointAddress.data(), ipv6Address->sin6_addr.s6_addr, sizeof(ipv6Address->sin6_addr.s6_addr));
		#endif
	}

	return socket;
}

word Socket::read(uint8* buffer, word bufferSize) {
	if (!this->connected)
		return 0;

	int received = ::recv(this->rawSocket, reinterpret_cast<char*>(buffer), static_cast<int>(bufferSize), 0);
	if (received <= 0)
		return 0;

	return static_cast<word>(received);
}

word Socket::write(const uint8* toWrite, word writeAmount) {
	if (!this->connected || writeAmount == 0)
		return 0;

	return static_cast<word>(::send(this->rawSocket, reinterpret_cast<const char*>(toWrite), static_cast<int>(writeAmount), 0));
}

array<uint8, Socket::ADDRESS_LENGTH> Socket::getRemoteAddress() const {
	return this->remoteEndpointAddress;
}

bool Socket::isConnected() const {
	return this->connected;
}

bool Socket::isDataAvailable() const {
	if (!this->connected)
		return false;

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


int16 Utilities::Net::hostToNetworkInt16(int16 value) {
	return htons(value);
}

int32 Utilities::Net::hostToNetworkInt32(int32 value) {
	return htonl(value);
}

int64 Utilities::Net::hostToNetworkInt64(int64 value) {
#ifdef WINDOWS
	return htonll(value);
#elif defined POSIX
	return htobe64(value); // XXX: glibc only
#endif
}

int16 Utilities::Net::networkToHostInt16(int16 value) {
	return ntohs(value);
}

int32 Utilities::Net::networkToHostInt32(int32 value) {
	return ntohl(value);
}

int64 Utilities::Net::networkToHostInt64(int64 value) {
#ifdef WINDOWS
	return ntohll(value);
#elif defined POSIX
	return be64toh(value); // XXX: glibc only
#endif
}