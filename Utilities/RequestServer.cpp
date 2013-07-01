#include "RequestServer.h"

using namespace Utilities;
using namespace std;

RequestServer::RequestServer(string port, uint8 workers, bool usesWebSockets, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state) {
	this->running = true;
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
	this->onRequest = onRequest;
	this->retryCode = retryCode;
	this->state = state;

	this->servers.push_back(new Net::TCPServer(port, usesWebSockets, this, onClientConnect, onRequestReceived));

	for (uint8 i = 0; i < workers; i++)
		this->workers.push_back(thread(&RequestServer::workerRun, this, i));

	this->outgoingWorker = thread(&RequestServer::outgoingWorkerRun, this);
}

RequestServer::RequestServer(vector<string> ports, uint8 workers, vector<bool> usesWebSockets, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state) {
	this->running = true;
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
	this->onRequest = onRequest;
	this->retryCode = retryCode;
	this->state = state;
	
	for (uint8 i = 0; i < ports.size(); i++)
		this->servers.push_back(new Net::TCPServer(ports[i], usesWebSockets[i], this, onClientConnect, onRequestReceived));

	for (uint8 i = 0; i < workers; i++)
		this->workers.push_back(thread(&RequestServer::workerRun, this, i));

	this->outgoingWorker = thread(&RequestServer::outgoingWorkerRun, this);
}

RequestServer::~RequestServer() {
	this->running = false;

	for (auto& i : this->workers)
		i.join();

	this->outgoingWorker.join();

	for (auto& i : this->clients)
			delete i.second;

	for (auto& i : this->servers)
		delete i;
}

void* RequestServer::onClientConnect(Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) {
	auto& requestServer = *reinterpret_cast<RequestServer*>(serverState);
	auto client = new RequestServer::Client(connection, requestServer, clientAddress);

	requestServer.clientListLock.lock();
	requestServer.clients[client->id] = client;
	requestServer.clientListLock.unlock();

	if (requestServer.onConnect)
		client->state = requestServer.onConnect(*client, requestServer.state);
	
	return client;
}

void RequestServer::onRequestReceived(Net::TCPConnection& connection, void* state, Net::TCPConnection::Message& message) {
	Client& client = *reinterpret_cast<Client*>(state);
	RequestServer& requestServer = client.parent;
	
	if (message.length == 0) {
		requestServer.clientListLock.lock();
		requestServer.clients.erase(client.id);
		requestServer.clientListLock.unlock();
		requestServer.onDisconnect(client, requestServer.state);
		delete &client;
	}
	else {
		requestServer.addToIncomingQueue(new Message(client, DataStream(message.data, message.length)));
	}
}

void RequestServer::workerRun(uint8 workerNumber) {
	uint16 requestId;
	uint8 requestCategory;
	uint8 requestMethod;
	bool wasHandled;
	Message* request;
	Message* response;

	while (this->running) {
		if (!this->incomingQueue.dequeue(request, 1000))
			continue;

		if (request->data.getLength() < 4) {
			delete request;
			continue; //maybe we should return an error?
		}

		requestId = request->data.read<uint16>();
		requestCategory = request->data.read<uint8>();
		requestMethod = request->data.read<uint8>();

		response = new Message(request->client, requestId);

		wasHandled = this->onRequest(workerNumber, request->client, requestCategory, requestMethod, request->data, response->data, this->state);

		if (!wasHandled) {
			if (request->currentAttempts++ < RequestServer::MAX_RETRIES) {
				response->data.write(this->retryCode);
			}
			else {
				this->addToIncomingQueue(request);
				continue;
			}
		}

		this->addToOutgoingQueue(response);
		delete request;
	}
}

void RequestServer::outgoingWorkerRun() {
	Message* message;

	while (this->running) {
		if (!this->outgoingQueue.dequeue(message, 1000))
			continue;
			
		message->client.connection.send(message->data.getBuffer(), message->data.getLength());

		delete message;
	}
}

void RequestServer::addToIncomingQueue(Message* const message) {
	this->incomingQueue.enqueue(message);
}

void RequestServer::addToOutgoingQueue(Message* const message) {
	this->outgoingQueue.enqueue(message);
}

RequestServer::Client::Client(Net::TCPConnection& connection, RequestServer& parent, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) : parent(parent), connection(connection) {
	this->state = nullptr;
	this->id = parent.nextId++;
	memcpy(this->ipAddress, clientAddress, Net::Socket::ADDRESS_LENGTH);
}

RequestServer::Message::Message(Client& client, const DataStream& message) : client(client), data(message) {
	this->currentAttempts = 0;
}

RequestServer::Message::Message(Client& client, uint16 id, uint8 category, uint8 method) : client(client) {
	RequestServer::Message::getHeader(this->data, id, category, method);
	this->currentAttempts = 0;
}

void RequestServer::Message::getHeader(DataStream& stream, uint16 id, uint8 category, uint8 method) {
	stream.write(id);
	stream.write(category);
	stream.write(method);
}