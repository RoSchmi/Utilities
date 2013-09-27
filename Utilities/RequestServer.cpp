#include "RequestServer.h"

using namespace Utilities;
using namespace std;

RequestServer::RequestServer(string port, bool usesWebSockets, uint8 workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state) : server(port, RequestServer::onClientConnect, this, usesWebSockets) {
	this->running = true;
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
	this->onRequest = onRequest;
	this->retryCode = retryCode;
	this->state = state;

	for (uint8 i = 0; i < workers; i++)
		this->incomingWorkers.push_back(thread(&RequestServer::incomingWorkerRun, this, i));

	this->outgoingWorker = thread(&RequestServer::outgoingWorkerRun, this);
	this->ioWorker = thread(&RequestServer::ioWorkerRun, this);
}

RequestServer::~RequestServer() {
	this->running = false;

	this->ioWorker.join();

	for (auto& i : this->incomingWorkers)
		i.join();

	this->outgoingWorker.join();
}

void RequestServer::onClientConnect(Net::TCPConnection&& connection, void* serverState) {
	auto& self = *reinterpret_cast<RequestServer*>(serverState);

	self.clientListLock.lock();

	self.clients.push_front(std::move(connection));
	if (self.onConnect)
		self.onConnect(self.clients.front(), self.state);

	self.clientListLock.unlock();
}

void RequestServer::incomingWorkerRun(uint8 workerNumber) {
	while (this->running) {
		unique_lock<mutex> lock(this->incomingLock);

		while (this->incomingQueue.empty())
			if (this->incomingCV.wait_for(lock, chrono::milliseconds(5000)) == cv_status::timeout)
				continue;
		
		Message request = this->incomingQueue.front();
		this->incomingQueue.pop();

		lock.unlock();

		if (request.data.getLength() < 4)
			continue; //maybe we should return an error?

		uint16 requestId = request.data.read<uint16>();
		uint8 requestCategory = request.data.read<uint8>();
		uint8 requestMethod = request.data.read<uint8>();
		Message response(request.connection, requestId);

		if (!this->onRequest(request.connection, this->state, workerNumber, requestCategory, requestMethod, request.data, response.data)) {
			if (request.currentAttempts++ < RequestServer::MAX_RETRIES) {
				response.data.write(this->retryCode);
			}
			else {
				this->addToIncomingQueue(request);
				continue;
			}
		}

		this->addToOutgoingQueue(response);
	}
}

void RequestServer::outgoingWorkerRun() {
	while (this->running) {
		unique_lock<mutex> lock(this->outgoingLock);

		while (this->outgoingQueue.empty())
			if (this->outgoingCV.wait_for(lock, chrono::milliseconds(5000)) == cv_status::timeout)
				continue;

		Message message = this->outgoingQueue.front();
		this->outgoingQueue.pop();

		lock.unlock();
			
		message.connection.send(message.data.getBuffer(), static_cast<uint16>(message.data.getLength()));
	}
}

void RequestServer::ioWorkerRun() {
	while (this->running) {
		this->clientListLock.lock();

		for (auto& i : this->clients)
			if (i.isDataAvailable()) 
				for (auto& k : i.read())
					this->addToIncomingQueue(Message(i, k.data, k.length));

		this->clientListLock.unlock();

		this_thread::sleep_for(chrono::microseconds(500));
	}
}

void RequestServer::addToIncomingQueue(Message message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->incomingLock);
	this->incomingQueue.push(message);
	this->incomingCV.notify_one();
}

void RequestServer::addToOutgoingQueue(Message message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->outgoingLock);
	this->outgoingQueue.push(message);
	this->outgoingCV.notify_one();
}

RequestServer::Message::Message(Net::TCPConnection& connection, const uint8* data, uint64 length) : connection(connection), data(data, length) {
	this->currentAttempts = 0;
}

RequestServer::Message::Message(Net::TCPConnection& connection, uint16 id, uint8 category, uint8 method) : connection(connection) {
	RequestServer::Message::writeHeader(this->data, id, category, method);
	this->currentAttempts = 0;
}

void RequestServer::Message::writeHeader(DataStream& stream, uint16 id, uint8 category, uint8 method) {
	stream.write(id);
	stream.write(category);
	stream.write(method);
}