#include "RequestServer.h"

#include <utility>
#include <stdexcept>

using namespace std;
using namespace util;
using namespace util::net;

request_server::request_server() {
	this->running = false;
	this->valid = false;
}

request_server::request_server(string port, bool usesWebSockets, word workers, uint16 retryCode, on_request_callback onRequest, on_connect_callback onConnect, on_disconnect_callback onDisconnect, void* state) : request_server(vector<string>{ port }, vector<bool>{ usesWebSockets }, workers, retryCode, onRequest, onConnect, onDisconnect, state) {
	
}

request_server::request_server(vector<string> ports, vector<bool> usesWebSockets, word workers, uint16 retryCode, on_request_callback onRequest, on_connect_callback onConnect, on_disconnect_callback onDisconnect, void* state) {
	this->running = false;
	this->valid = true;
	this->onConnect = onConnect;
	this->onDisconnect = onDisconnect;
	this->onRequest = onRequest;
	this->retryCode = retryCode;
	this->state = state;
	this->workers = workers;

	for (word i = 0; i < ports.size(); i++)
		this->servers.emplace_back(ports[i], request_server::onClientConnect, this, usesWebSockets[i]);
}

request_server::request_server(request_server&& other) {
	if (other.running)
		throw runtime_error("Cannot move a running request_server.");

	this->running = false;
	*this = std::move(other);
}

request_server& request_server::operator = (request_server&& other) {
	if (other.running || this->running)
		throw runtime_error("Cannot move to or from a running request_server.");

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

request_server::~request_server() {
	this->stop();
}

void request_server::start() {
	if (!this->valid)
		throw runtime_error("Default-Constructed RequestServers cannot be started.");

	if (this->running)
		return;

	for (word i = 0; i < this->workers; i++)
		this->incomingWorkers.push_back(thread(&request_server::incomingWorkerRun, this, i));

	this->outgoingWorker = thread(&request_server::outgoingWorkerRun, this);
	this->ioWorker = thread(&request_server::ioWorkerRun, this);

	for (auto& i : this->servers)
		i.start();
}

void request_server::stop() {
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

tcp_connection& request_server::adoptConnection(tcp_connection&& connection, bool callOnClientConnect) {
	this->clientListLock.lock();

	this->clients.push_back(std::move(connection));
	auto& newReference = this->clients.back();

	if (callOnClientConnect && this->onConnect)
		this->onConnect(newReference, this->state);

	this->clientListLock.unlock();

	return newReference;
}

void request_server::onClientConnect(tcp_connection&& connection, void* serverState) {
	auto& self = *reinterpret_cast<request_server*>(serverState);

	self.clientListLock.lock();

	self.clients.push_back(std::move(connection));
	if (self.onConnect)
		self.onConnect(self.clients.back(), self.state);

	self.clientListLock.unlock();
}

void request_server::incomingWorkerRun(word workerNumber) {
	while (this->running) {
		this_thread::sleep_for(chrono::microseconds(500));

		unique_lock<mutex> lock(this->incomingLock);

		while (this->incomingQueue.empty())
			if (this->incomingCV.wait_for(lock, chrono::milliseconds(5000)) == cv_status::timeout)
				continue;
		
		message request(std::move(this->incomingQueue.front()));
		this->incomingQueue.pop();

		lock.unlock();

		if (request.data.getLength() < 4)
			continue;

		uint16 requestId;
		uint8 requestCategory, requestMethod;
		request.data >> requestId >> requestCategory >> requestMethod;

		message response(request.connection, requestId);

		switch (this->onRequest(request.connection, this->state, workerNumber, requestCategory, requestMethod, request.data, response.data)) {
			case request_result::SUCCESS:
				this->addToOutgoingQueue(std::move(response));

				break;
			case request_result::RETRY_LATER:
				if (++request.currentAttempts >= request_server::MAX_RETRIES)
					this->addToIncomingQueue(std::move(request));
				else
					response.data.write(this->retryCode);

				break;
			case request_result::NO_RESPONSE:
				break;
		}
	}
}

void request_server::outgoingWorkerRun() {
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

void request_server::ioWorkerRun() {
	while (this->running) {
		this->clientListLock.lock();

		for (auto& i : this->clients)
			if (i.isDataAvailable()) 
				for (auto& k : i.read())
					this->addToIncomingQueue(message(i, std::move(k)));

		this->clientListLock.unlock();

		this_thread::sleep_for(chrono::microseconds(500));
	}
}

void request_server::addToIncomingQueue(message&& message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->incomingLock);
	this->incomingQueue.push(std::move(message));
	this->incomingCV.notify_one();
}

void request_server::addToOutgoingQueue(message&& message) {
	if (!this->running)
		return;

	unique_lock<mutex> lock(this->outgoingLock);
	this->outgoingQueue.push(std::move(message));
	this->outgoingCV.notify_one();
}

request_server::message::message(tcp_connection& connection, tcp_connection::message&& message) : connection(connection), data(message.data, message.length) {
	this->currentAttempts = 0;
}

request_server::message::message(tcp_connection& connection, data_stream&& data) : connection(connection), data(std::move(data)) {
	this->currentAttempts = 0;
}

request_server::message::message(tcp_connection& connection, const uint8* data, word length) : connection(connection), data(data, length) {
	this->currentAttempts = 0;
}

request_server::message::message(tcp_connection& connection, uint16 id, uint8 category, uint8 method) : connection(connection) {
	request_server::message::writeHeader(this->data, id, category, method);
	this->currentAttempts = 0;
}

request_server::message::message(request_server::message&& other) : connection(other.connection), data(std::move(other.data)) {
	this->currentAttempts = other.currentAttempts;
}

void request_server::message::writeHeader(data_stream& stream, uint16 id, uint8 category, uint8 method) {
	stream << id << category << method;
}