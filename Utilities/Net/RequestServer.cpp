#include "RequestServer.h"

#include <utility>

using namespace std;
using namespace util;
using namespace util::net;

request_server::request_server() {
	this->running = false;
	this->valid = false;
}

request_server::request_server(string port, word workers, uint16 retry_code, bool uses_websockets) : request_server(vector<string>{ port }, workers, retry_code, vector<bool>{ uses_websockets }) {
	
}

request_server::request_server(vector<string> ports, word workers, uint16 retry_code, vector<bool> uses_websockets) {
	this->running = false;
	this->valid = true;
	this->retry_code = retry_code;
	this->state = nullptr;
	this->workers = workers;

	for (word i = 0; i < ports.size(); i++) {
		this->servers.emplace_back(ports[i], uses_websockets[i]);
		auto& server = this->servers.back();
		server.on_connect += request_server::on_client_connect;
		server.state = this;
	}
}

request_server::request_server(request_server&& other) {
	if (other.running)
		throw cant_move_running_server_exception();

	this->running = false;
	*this = move(other);
}

request_server& request_server::operator = (request_server&& other) {
	if (other.running || this->running)
		throw cant_move_running_server_exception();

	this->valid = other.valid.load();
	this->retry_code = other.retry_code;
	this->state = other.state;
	this->workers = other.workers;
	this->running = false;
	this->servers = move(other.servers);

	return *this;
}

request_server::~request_server() {
	this->stop();
}

void request_server::start() {
	if (!this->valid)
		throw cant_start_default_constructed_exception();

	if (this->running)
		return;

	for (word i = 0; i < this->workers; i++)
		this->incoming_workers.push_back(thread(&request_server::incoming_run, this, i));

	this->outgoing_worker = thread(&request_server::outgoing_run, this);
	this->io_worker = thread(&request_server::io_run, this);

	for (auto& i : this->servers)
		i.start();
}

void request_server::stop() {
	if (!this->running)
		return;

	for (auto& i : this->servers)
		i.stop();

	this->running = false;

	this->io_worker.join();

	for (auto& i : this->incoming_workers)
		i.join();

	this->outgoing_worker.join();
}

tcp_connection& request_server::adopt(tcp_connection&& connection, bool call_on_connect) {
	this->client_lock.lock();

	this->clients.push_back(move(connection));
	auto& ref = this->clients.back();

	if (call_on_connect)
		this->on_connect(ref, this->state);

	this->client_lock.unlock();

	return ref;
}

void request_server::on_client_connect(tcp_connection&& connection, void* serverState) {
	auto& self = *reinterpret_cast<request_server*>(serverState);

	self.client_lock.lock();

	self.clients.push_back(move(connection));
	self.on_connect(self.clients.back(), self.state);
	self.client_lock.unlock();
}

void request_server::incoming_run(word worker_number) {
	message request;

	while (this->running) {
		this_thread::sleep_for(chrono::microseconds(500));

		this->incoming.dequeue(request, chrono::seconds(5));

		if (request.data.size() < 4)
			continue;

		uint16 id;
		uint8 category, method;
		request.data >> id >> category >> method;

		message response(request.connection, id);

		switch (this->on_request(request.connection, worker_number, category, method, request.data, response.data, this->state)) {
			case request_result::success:
				this->enqueue_outgoing(move(response));

				break;
			case request_result::retry_later:
				if (++request.attempts >= request_server::MAX_RETRIES)
					this->enqueue_incoming(move(request));
				else
					response.data.write(this->retry_code);

				break;
			case request_result::no_response:
				break;
		}
	}
}

void request_server::outgoing_run() {
	message m;

	while (this->running) {
		this_thread::sleep_for(chrono::microseconds(500));
		this->outgoing.dequeue(m, chrono::seconds(5));
		m.connection.send(m.data.data(), m.data.size());
	}
}

void request_server::io_run() {
	while (this->running) {
		this->client_lock.lock();

		for (auto& i : this->clients)
			if (i.data_available()) 
				for (auto& k : i.read())
					this->enqueue_incoming(message(i, move(k)));

		this->client_lock.unlock();

		this_thread::sleep_for(chrono::microseconds(500));
	}
}

void request_server::enqueue_incoming(message m) {
	if (!this->running)
		return;

	this->incoming.enqueue(move(m));
}

void request_server::enqueue_outgoing(message m) {
	if (!this->running)
		return;

	this->incoming.enqueue(move(m));
}

request_server::message::message(tcp_connection& connection, tcp_connection::message message) : connection(connection), data(message.data, message.length) {
	this->attempts = 0;
}

request_server::message::message(tcp_connection& connection, data_stream&& data) : connection(connection), data(move(data)) {
	this->attempts = 0;
}

request_server::message::message(tcp_connection& connection, const uint8* data, word length) : connection(connection), data(data, length) {
	this->attempts = 0;
}

request_server::message::message(tcp_connection& connection, uint16 id, uint8 category, uint8 method) : connection(connection) {
	request_server::message::write_header(this->data, id, category, method);
	this->attempts = 0;
}

request_server::message::message(request_server::message&& other) : connection(other.connection), data(move(other.data)) {
	this->attempts = other.attempts;
}

void request_server::message::write_header(data_stream& stream, uint16 id, uint8 category, uint8 method) {
	stream << id << category << method;
}