#include "RequestServer.h"

#include <utility>
#include <stdexcept>

using namespace std;
using namespace Utilities;
using namespace Utilities::Net;

RequestServer::RequestServer() {
	this->running = false;
	this->valid = false;
}

RequestServer::RequestServer(string port, bool usesWebSockets, word workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state) : RequestServer(vector<string>{ port }, vector<bool>{ usesWebSockets }, workers, retryCode, onRequest, onConnect, onDisconnect, state) {
	
}

RequestServer::RequestServer(vector<string> ports, vector<bool> usesWebSockets, word workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state) {
	this->running = false;
	this->valid = true;
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
	this->onRequest = onRequest;
	this->retryCode = retryCode;
	this->state = state;
	this->workers = workers;

	for (word i = 0; i < ports.size(); i++)
		this->servers.emplace_back(ports[i], RequestServer::onClientConnect, this, usesWebSockets[i]);
}

RequestServer::RequestServer(RequestServer&& other) {
	if (other.running)
		throw runtime_error("Cannot move a running RequestServer.");

	this->running = false;
	*this = std::move(other);
}

RequestServer& RequestServer::operator = (RequestServer&& other) {
	if (other.running || this->running)
		throw runtime_error("Cannot move to or from a running RequestServer.");

	this->valid = other.valid.load();
	this->onConnect = other.onConnect;
	this->onDisconnect = other.onDisconnect;
	this->onRequest = other.onRequest;
	this->retryCode = other.retryCode;
	this->state = other.state;
	this->workers = other.workers;
	this->running = false;
	this->servers = std::move(other.servers);

	return *this;
}

RequestServer::~RequestServer() {
	this->stop();
}

void RequestServer::start() {
	if (!this->valid)
		throw runtime_error("Default-Constructed RequestServers cannot be started.");

	if (this->running)
		return;

	for (word i = 0; i < this->workers; i++)
		this->incomingWorkers.push_back(thread(&RequestServer::incomingWorkerRun, this, i));

	this->outgoingWorker = thread(&RequestServer::outgoingWorkerRun, this);
	this->ioWorker = thread(&RequestServer::ioWorkerRun, this);

	for (auto& i : this->servers)
		i.start();
}

void RequestServer::stop() {
	if (!this->running)
		return;

	for (auto& i : this->servers)
		i.stop();

	this->running = false;

	this->ioWorker.join();

	for (auto& i : this->incomingWorkers)
		i.join();

	this->outgoingWorker.join();
}

void RequestServer::adoptConnection(TCPConnection&& connection) {
	RequestServer::onClientConnect(std::move(connection), this);
}

void RequestServer::onClientConnect(TCPConnection&& connection, void* serverState) {
	auto& self = *reinterpret_cast<RequestServer*>(serverState);

	self.clientListLock.lock();

	self.clients.push_back(std::move(connection));
	if (self.onConnect)
		self.onConnect(self.clients.back(), self.state);

	self.clientListLock.unlock();
}

void RequestServer::incomingWorkerRun(word workerNumber) {
	while (this->running) {
		this_thread::sleep_for(chrono::microseconds(500));

		unique_lock<mutex> lock(this->incomingLock);

		while (this->incomingQueue.empty())
			if (this->incomingCV.wait_for(lock, chrono::milliseconds(5000)) == cv_status::timeout)
				continue;
		
		Message request(std::move(this->incomingQueue.front()));
		this->incomingQueue.pop();

		lock.unlock();

		if (request.data.getLength() < 4)
			continue;

		uint16 requestId = request.data.read<uint16>();
		uint8 requestCategory = request.data.read<uint8>();
		uint8 requestMethod = request.data.read<uint8>();
		Message response(request.connection, requestId);

		if (!this->onRequest(request.connection, this->state, workerNumber, requestCategory, requestMethod, request.data, response.data)) {
			if (request.currentAttempts++ < RequestServer::MAX_RETRIES) {
				response.data.write(this->retryCode);
			}
			else {
				this->addToIncomingQueue(std::move(request));
				continue;
			}
		}

		this->addToOutgoingQueue(std::move(response));
	}
}

void RequestServer::outgoingWorkerRun() {
	while (this->running) {
		this_thread::sleep_for(chrono::microseconds(500));

		unique_lock<mutex> lock(this->outgoingLock);

		while (this->outgoingQueue.empty())
			if (this->outgoingCV.wait_for(lock, chrono::milliseconds(5000)) == cv_status::timeout)
				continue;

		auto& message = this->outgoingQueue.front();
		message.connection.send(message.data.getBuffer(), static_cast<uint16>(message.data.getLength()));
		this->outgoingQueue.pop();
	}
}

void RequestServer::ioWorkerRun() {
	while (this->running) {
		this->clientListLock.lock();

		for (auto& i : this->clients)
			if (i.isDataAvailable()) 
				for (auto& k : i.read())
					this->addToIncomingQueue(Message(i, std::move(k)));

		this->clientListLock.unlock();

		this_thread::sleep_for(chrono::microseconds(500));
	}
}

void RequestServer::addToIncomingQueue(Message&& message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->incomingLock);
	this->incomingQueue.push(std::move(message));
	this->incomingCV.notify_one();
}

void RequestServer::addToOutgoingQueue(Message&& message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->outgoingLock);
	this->outgoingQueue.push(std::move(message));
	this->outgoingCV.notify_one();
}

RequestServer::Message::Message(TCPConnection& connection, TCPConnection::Message&& message) : connection(connection), data(message.data, message.length) {
	this->currentAttempts = 0;
}

RequestServer::Message::Message(TCPConnection& connection, DataStream&& data) : connection(connection), data(std::move(data)) {
	this->currentAttempts = 0;
}

RequestServer::Message::Message(TCPConnection& connection, const uint8* data, word length) : connection(connection), data(data, length) {
	this->currentAttempts = 0;
}

RequestServer::Message::Message(TCPConnection& connection, uint16 id, uint8 category, uint8 method) : connection(connection) {
	RequestServer::Message::writeHeader(this->data, id, category, method);
	this->currentAttempts = 0;
}

RequestServer::Message::Message(RequestServer::Message&& other) : connection(other.connection), data(std::move(other.data)) {
	this->currentAttempts = other.currentAttempts;
}

void RequestServer::Message::writeHeader(DataStream& stream, uint16 id, uint8 category, uint8 method) {
	stream.write(id);
	stream.write(category);
	stream.write(method);
}