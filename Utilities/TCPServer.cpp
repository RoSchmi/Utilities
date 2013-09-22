#include "TCPServer.h"
#include "WebSocketConnection.h"

#include <utility>
#include <algorithm>

using namespace std;
using namespace Utilities::Net;

TCPServer::TCPServer(string port, bool isWebSocket, void* onConnectState, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback) : listener(Socket::Families::IPAny, Socket::Types::TCP, port) {
	this->isWebSocket = isWebSocket;
	this->active = true;
	this->receiveCallback = receiveCallback;
	this->connectCallback = connectCallback;
	this->state = onConnectState;
	this->acceptWorker = thread(&TCPServer::acceptWorkerRun, this);
	this->asyncWorker = thread(&TCPServer::asyncWorkerRun, this);
}

TCPServer::~TCPServer() {
	this->shutdown();
}

void TCPServer::shutdown() {
	if (!this->active)
		return;

	this->active = false;
	this->listener.close();

	while (!this->clientList.empty()) {
		auto i = this->clientList.back();
		this->clientList.pop_back();
		i->disconnect(false);
		delete i;
	}

	this->acceptWorker.join();
	this->asyncWorker.join();
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
		}
	}
}

void TCPServer::asyncWorkerRun() {
	while (this->active) {
		this->clientListLock.lock();

		for (auto i : this->clientList)
			if (i->getBaseSocket().isDataAvailable())
				for (auto& j : i->read())
					this->receiveCallback(*i, i->state, j);

		this->clientListLock.unlock();
	}
}

void TCPServer::onClientDisconnecting(TCPConnection* client) {
	this->clientListLock.lock();

	auto position = find(this->clientList.begin(), this->clientList.end(), client);
	if (position != this->clientList.end())
		this->clientList.erase(position, position + 1);

	this->clientListLock.unlock();
}
