#include "TCPServer.h"

#include <utility>

#include "WebSocketConnection.h"

using namespace std;
using namespace util;
using namespace util::net;

tcp_server::tcp_server() {
	this->active = false;
	this->valid = false;
}

tcp_server::tcp_server(string port, bool is_websocket) {
	this->is_websocket = is_websocket;
	this->active = false;
	this->port = port;
	this->valid = true;
}

tcp_server::tcp_server(tcp_server&& other) {
	if (other.active)
		throw cant_move_running_server_exception();

	this->active = false;
	*this = move(other);
}

tcp_server& tcp_server::operator=(tcp_server&& other) {
	if (other.active || this->active)
		throw cant_move_running_server_exception();

	this->is_websocket = other.is_websocket;
	this->valid = other.valid.load();
	this->port = other.port;
	this->active = false;

	return *this;
}

tcp_server::~tcp_server() {
	this->stop();
}

void tcp_server::start() {
	if (!this->valid)
		throw cant_start_default_constructed_exception();

	if (this->active)
		return;

	this->active = true;
	this->listener = socket(socket::families::ip_any, socket::types::tcp, this->port);
	this->accept_worker = thread(&tcp_server::accept_worker_run, this);
}

void tcp_server::stop() {
	if (!this->active)
		return;

	this->active = false;
	this->listener.close();
	this->accept_worker.join();
}

void tcp_server::accept_worker_run() {
	while (this->active) {
		socket acceptedSocket = this->listener.accept();
		if (acceptedSocket.is_connected())
			this->on_connect(!this->is_websocket ? tcp_connection(move(acceptedSocket)) : websocket_connection(move(acceptedSocket)));
	}
}
