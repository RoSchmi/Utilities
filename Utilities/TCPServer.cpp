#include "TCPServer.h"

#include <stdexcept>
#include <utility>

#include "WebSocketConnection.h"

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

TCPServer::TCPServer() {
	this->active = false;
	this->valid = false;
}

TCPServer::TCPServer(string port, OnConnectCallback connectCallback, void* onConnectState, bool isWebSocket) {
	this->isWebSocket = isWebSocket;
	this->active = false;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->port = port;
	this->valid = true;
}

TCPServer::TCPServer(TCPServer&& other) {
	if (other.active)
		throw runtime_error("Cannot move a running TCPServer.");

	this->active = false;
	*this = std::move(other);
}

TCPServer& TCPServer::operator = (TCPServer&& other) {
	if (other.active || this->active)
		throw runtime_error("Cannot move to or from a running TCPServer.");

	this->isWebSocket = other.isWebSocket;
	this->valid = other.valid.load();
	this->connectCallback = other.connectCallback;
	this->state = other.state;
	this->port = other.port;
	this->active = false;

	return *this;
}

TCPServer::~TCPServer() {
	this->stop();
}

void TCPServer::start() {
	if (!this->valid)
		throw runtime_error("Default-Constructed TCPServer cannot be started.");

	if (this->active)
		return;

	this->active = true;
	this->listener = Socket(Socket::Families::IPAny, Socket::Types::TCP, this->port);
	this->acceptWorker = thread(&TCPServer::acceptWorkerRun, this);
}

void TCPServer::stop() {
	if (!this->active)
		return;

	this->active = false;
	this->listener.close();
	this->acceptWorker.join();
}

void TCPServer::acceptWorkerRun() {
	while (this->active) {
		Socket acceptedSocket = this->listener.accept();
		if (acceptedSocket.isConnected())
			this->connectCallback(!this->isWebSocket ? TCPConnection(std::move(acceptedSocket)) : WebSocketConnection(std::move(acceptedSocket)), this->state);
	}
}
