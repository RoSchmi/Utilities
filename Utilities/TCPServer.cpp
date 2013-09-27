#include "TCPServer.h"

#include "WebSocketConnection.h"

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

TCPServer::TCPServer(string port, OnConnectCallback connectCallback, void* onConnectState, bool isWebSocket) : listener(Socket::Families::IPAny, Socket::Types::TCP, port) {
	this->isWebSocket = isWebSocket;
	this->active = true;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->acceptWorker = thread(&TCPServer::acceptWorkerRun, this);
}

TCPServer::~TCPServer() {
	this->close();
}

void TCPServer::close() {
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
