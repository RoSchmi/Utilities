#include "TCPServer.h"

#include <stdexcept>
#include <utility>

#include "WebSocketConnection.h"

using namespace std;
using namespace util;
using namespace util::net;

tcp_server::tcp_server() {
	this->active = false;
	this->valid = false;
}

tcp_server::tcp_server(string port, on_connect_callback connectCallback, void* onConnectState, bool isWebSocket) {
	this->isWebSocket = isWebSocket;
	this->active = false;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->port = port;
	this->valid = true;
}

tcp_server::tcp_server(tcp_server&& other) {
	if (other.active)
		throw runtime_error("Cannot move a running tcp_server.");

	this->active = false;
	*this = std::move(other);
}

tcp_server& tcp_server::operator = (tcp_server&& other) {
	if (other.active || this->active)
		throw runtime_error("Cannot move to or from a running tcp_server.");

	this->isWebSocket = other.isWebSocket;
	this->valid = other.valid.load();
	this->connectCallback = other.connectCallback;
	this->state = other.state;
	this->port = other.port;
	this->active = false;

	return *this;
}

tcp_server::~tcp_server() {
	this->stop();
}

void tcp_server::start() {
	if (!this->valid)
		throw runtime_error("Default-Constructed tcp_server cannot be started.");

	if (this->active)
		return;

	this->active = true;
	this->listener = socket(socket::families::IPAny, socket::types::TCP, this->port);
	this->acceptWorker = thread(&tcp_server::acceptWorkerRun, this);
}

void tcp_server::stop() {
	if (!this->active)
		return;

	this->active = false;
	this->listener.close();
	this->acceptWorker.join();
}

void tcp_server::acceptWorkerRun() {
	while (this->active) {
		socket acceptedSocket = this->listener.accept();
		if (acceptedSocket.isConnected())
			this->connectCallback(!this->isWebSocket ? tcp_connection(std::move(acceptedSocket)) : websocket_connection(std::move(acceptedSocket)), this->state);
	}
}
