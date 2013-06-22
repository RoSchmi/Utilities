#include "RequestServer.h"

using namespace Utilities;
using namespace std;

RequestServer::RequestServer(string port, uint8 workers, bool usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state) {
	this->running = true;
	this->handler = handler;
	this->retryCode = retryCode;
	this->state = state;

	this->servers.push_back(new Net::TCPServer(port, usesWebSockets, this, onClientConnect, onRequestReceived));

	for (uint8 i = 0; i < workers; i++)
		this->workers.push_back(thread(&RequestServer::workerRun, this, i));

	this->outgoingWorker = thread(&RequestServer::outgoingWorkerRun, this);
}

RequestServer::RequestServer(vector<string> ports, uint8 workers, vector<bool> usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state) {
	this->running = true;
	this->handler = handler;
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
		for (auto j : i.second)
			delete j.second;

	for (auto& i : this->servers)
		delete i;
}

void* RequestServer::onClientConnect(Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) {
	auto& requestServer = *reinterpret_cast<RequestServer*>(serverState);
	auto id = requestServer.nextId++;
	auto client = new RequestServer::Client(connection, requestServer, clientAddress, id);
	
	requestServer.clientListLock.lock();
	requestServer.clients[0][id] = client;
	requestServer.clientListLock.unlock();
	
	return client;
}

void RequestServer::onRequestReceived(Net::TCPConnection& connection, void* state, Net::TCPConnection::Message& message) {
	RequestServer::Client& client = *reinterpret_cast<RequestServer::Client*>(state);
	RequestServer& requestServer = client.parent;
	
	if (message.length == 0) {
		requestServer.clientListLock.lock();
		requestServer.clients[client.authenticatedId].erase(client.id);
		requestServer.clientListLock.unlock();
		delete &client;
		return;
	}
	
	requestServer.queue.enqueue(new RequestServer::Message(client, message));
	message.data = nullptr;
	message.length = 0;
}

void RequestServer::workerRun(uint8 workerNumber) {
	uint16 requestId;
	uint8 requestCategory;
	uint8 requestMethod;
	bool wasHandled;
	Message* request;
	uint64 startId;

	while (this->running) {
		if (!this->queue.dequeue(request, 1000))
			continue;

		if (request->data.getLength() < 4)
			continue; //maybe we should return an error?

		startId = request->client.authenticatedId;
		requestId = request->data.read<uint16>();
		requestCategory = request->data.read<uint8>();
		requestMethod = request->data.read<uint8>();

		this->response.reset();
		this->response.write<uint16>(requestId);
		wasHandled = this->handler(workerNumber, request->client, requestCategory, requestMethod, request->data, this->response, this->state);
		
		if (startId != request->client.authenticatedId) {
			this->clientListLock.lock();
			this->clients[startId].erase(request->client.id);
			this->clients[request->client.authenticatedId][request->client.id] = &request->client;
			this->clientListLock.unlock();
		}

		if (wasHandled) {
			request->client.connection.send(this->response.getBuffer(), static_cast<uint16>(this->response.getLength()));
			delete request;
		}
		else {
			request->currentAttempts++;
			if (request->currentAttempts < RequestServer::MAX_RETRIES) {
				this->queue.enqueue(request);
			}
			else {
				this->response.write<uint16>(this->retryCode);
				request->client.connection.send(this->response.getBuffer(), static_cast<uint16>(this->response.getLength()));
				delete request;
			}
		}
	}
}

void RequestServer::outgoingWorkerRun() {
	Message* message;
	uint8 requestId[2] = {0x00, 0x0};

	while (this->running) {
		if (!this->outgoingQueue.dequeue(message, 1000))
			continue;
			
		message->client.connection.addPart(requestId, 2);
		message->client.connection.addPart(message->data.getBuffer(), message->data.getLength());
		message->client.connection.sendParts();
		delete message;
	}
}

void RequestServer::send(uint64 authenticatedId, uint64 sourceConnectionId, DataStream& message) {
	this->clientListLock.lock();

	auto iter = this->clients.find(authenticatedId);
	if (iter != this->clients.end())
		for (auto i : iter->second)
			if (i.second->id != sourceConnectionId)
				this->outgoingQueue.enqueue(new RequestServer::Message(*i.second, message));

	this->clientListLock.unlock();
}
