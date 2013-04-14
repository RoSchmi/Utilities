#include "TCPServer.h"
#include "WebSocketConnection.h"
#include "MovableList.h"

#include <utility>
#include <algorithm>

using namespace std;
using namespace Utilities::Net;

TCPServer::TCPServer(string port, bool isWebSocket, void* onConnectState, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback) : listener(Socket::Families::IPAny, Socket::Types::TCP, port), asyncWorker(TCPServer::asyncReadCallback) {
	this->isWebSocket = isWebSocket;
	this->active = true;
	this->receiveCallback = receiveCallback;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->acceptWorker = thread(&TCPServer::acceptWorkerRun, this);
	this->asyncWorker.start();
}

TCPServer::~TCPServer() {
	for (TCPConnection* i : this->clientList) {
		i->disconnect(false);
		this->asyncWorker.unregisterSocket(i->connection);
		delete i;
	}

	this->active = false;
	this->listener.close();
	this->acceptWorker.join();
	this->asyncWorker.shutdown();
}

void TCPServer::acceptWorkerRun() {
	while (this->active) {
		Socket acceptedSocket = this->listener.accept();

		if (acceptedSocket.isConnected()) {
			TCPConnection* newClient;

			if (!this->isWebSocket)
				newClient = new TCPConnection(this, acceptedSocket);
			else
				newClient = new WebSocketConnection(this, acceptedSocket);
			
			const Socket& newSocket = newClient->getBaseSocket();

			if (this->connectCallback)
				newClient->state = this->connectCallback(*newClient, this->state, newSocket.getRemoteAddress());
			
			this->clientListLock.lock();
			this->clientList.push_back(newClient);
			this->clientListLock.unlock();

			this->asyncWorker.registerSocket(newSocket, newClient);
		}
	}
}

void TCPServer::asyncReadCallback(const Socket& socket, void* state) {
	TCPConnection& connection = *reinterpret_cast<TCPConnection*>(state);
	for (auto& i : connection.read())
		connection.owningServer->receiveCallback(connection, connection.state, i);
}

void TCPServer::onClientDisconnecting(TCPConnection* client) {
	this->clientListLock.lock();

	auto position = find(this->clientList.begin(), this->clientList.end(), client);
	if (position != this->clientList.end())
		this->clientList.erase(position, position + 1);

	this->clientListLock.unlock();
		
	this->asyncWorker.unregisterSocket(client->connection);
}
