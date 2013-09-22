#include "TCPServer.h"
#include "WebSocketConnection.h"

#include <utility>
#include <algorithm>

using namespace std;
using namespace Utilities::Net;

TCPServer::TCPServer(string port, bool isWebSocket, OnConnectCallback connectCallback, void* onConnectState) : listener(Socket::Families::IPAny, Socket::Types::TCP, port) {
	this->isWebSocket = isWebSocket;
	this->active = true;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->acceptWorker = thread(&TCPServer::acceptWorkerRun, this);
}

TCPServer::~TCPServer() {
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
			this->connectCallback(!this->isWebSocket ? TCPConnection(acceptedSocket) : WebSocketConnection(acceptedSocket), this->state);
	}
}
