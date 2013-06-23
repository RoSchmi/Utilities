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
	RequestServer::Client* client = reinterpret_cast<RequestServer::Client*>(state);
	RequestServer& requestServer = client->parent;
	
	if (message.length == 0) {
		requestServer.clientListLock.lock();
		requestServer.clients.erase(client->id);
		requestServer.clientListLock.unlock();
		requestServer.onDisconnect(*client, requestServer.state);
		delete client;
	}
	else {
		DataStream stream;
		stream.adopt(message.data, message.length);
		requestServer.queue.enqueue(new RequestServer::Message(*client, stream));
		message.data = nullptr;
		message.length = 0;
	}
}

void RequestServer::workerRun(uint8 workerNumber) {
	uint16 requestId;
	uint8 requestCategory;
	uint8 requestMethod;
	bool wasHandled;
	Message* request;

	while (this->running) {
		if (!this->queue.dequeue(request, 1000))
			continue;

		if (request->data.getLength() < 4)
			continue; //maybe we should return an error?

		requestId = request->data.read<uint16>();
		requestCategory = request->data.read<uint8>();
		requestMethod = request->data.read<uint8>();

		DataStream response;
		response.write<uint16>(requestId);
		wasHandled = this->onRequest(workerNumber, request->client, requestCategory, requestMethod, request->data, response, this->state);
		
		if (wasHandled) {
			this->send(request->client, response);
			delete request;
		}
		else {
			request->currentAttempts++;
			if (request->currentAttempts < RequestServer::MAX_RETRIES) {
				this->queue.enqueue(request);
			}
			else {
				response.reset();
				response.write<uint16>(requestId);
				response.write<uint16>(this->retryCode);
				this->send(request->client, response);
				delete request;
			}
		}
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

DataStream RequestServer::getOutOfBandMessageStream() const {
	DataStream stream;
	stream.write<uint16>(0);
	return stream;
}

void RequestServer::send(uint64 connectionId, DataStream& message) {
	this->clientListLock.lock();

	auto iter = this->clients.find(connectionId);
	if (iter != this->clients.end())
		this->send(*iter->second, message);

	this->clientListLock.unlock();
}

void RequestServer::send(Client& client, DataStream& message) {
	this->outgoingQueue.enqueue(new RequestServer::Message(client, message));
}

RequestServer::Client::Client(Net::TCPConnection& connection, RequestServer& parent, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) : parent(parent), connection(connection) {
	this->state = nullptr;
	this->id = parent.nextId++;
	memcpy(this->ipAddress, clientAddress, Net::Socket::ADDRESS_LENGTH);
}

RequestServer::Message::Message(Client& client, DataStream& message) : client(client), data(message) {
	this->currentAttempts = 0;
}