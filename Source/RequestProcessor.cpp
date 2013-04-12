#include "RequestProcessor.h"

using namespace Utilities;
using namespace std;

//test
//test1

RequestProcessor::RequestProcessor(const int8* port, uint8 workers, bool usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state) : server(port, usesWebSockets, this, onClientConnect, onRequestReceived) {
	this->running = true;
	this->handler = handler;
	this->retryCode = retryCode;
	this->state = state;

	for (uint8 i = 0; i < workers; i++)
		this->workers.push_back(thread(&RequestProcessor::workerRun, this));
}

RequestProcessor::~RequestProcessor() {
	this->running = false;

	for (auto& i : this->workers) {
		i.join();
	}
}

void* RequestProcessor::onClientConnect(Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) {
	return new RequestProcessor::Client(connection, *reinterpret_cast<RequestProcessor*>(serverState), clientAddress);
}

void RequestProcessor::onRequestReceived(Net::TCPConnection& connection, void* state, Net::TCPConnection::Message& message) {
	RequestProcessor::Client* client = reinterpret_cast<RequestProcessor::Client*>(state);
	RequestProcessor& requestProcessor = client->parent;

	if (message.length == 0) {
		delete client;
		return;
	}
	
	requestProcessor.requestQueue.enqueue(new RequestProcessor::Request(*client, message));
	message.data = nullptr;
	message.length = 0;
}

void RequestProcessor::workerRun() {
	uint16 requestId;
	uint8 requestCategory;
	uint8 requestMethod;
	bool wasHandled;
	Request* request;

	while (this->running) {
		if (!this->requestQueue.dequeue(request, 1000))
			continue;

		requestId =  request->parameters.read<uint16>();
		requestCategory = request->parameters.read<uint8>();
		requestMethod = request->parameters.read<uint8>();

		this->response.reset();
		this->response.write<uint16>(requestId);
		wasHandled = this->handler(request->client, requestCategory, requestMethod, request->parameters, this->response, this->state);
		
		if (wasHandled) {
			request->client.connection.send(this->response.getBuffer(), static_cast<uint16>(this->response.getLength()));
		}
		else {
			request->currentAttempts++;
			if (request->currentAttempts < RequestProcessor::MAX_RETRIES) {
				this->requestQueue.enqueue(request);
			}
			else {
				this->response.write<uint16>(this->retryCode);
				request->client.connection.send(this->response.getBuffer(), static_cast<uint16>(this->response.getLength()));
				delete request;
			}
		}
	}
}
