#include "RequestServer.h"

#include <utility>
#include <functional>
#include <memory>
#include <algorithm>

using namespace std;
using namespace util;
using namespace util::net;

request_server::request_server() : incoming(0) , outgoing(0) {
	this->running = false;
	this->valid = false;
}

request_server::request_server(endpoint port, word workers, uint16 retry_code) : request_server(vector<endpoint>{ port }, workers, retry_code) {
	
}

request_server::request_server(vector<endpoint> ports, word workers, uint16 retry_code) : incoming(workers) , outgoing(workers) {
	this->running = false;
	this->valid = true;
	this->retry_code = retry_code;

	for (word i = 0; i < ports.size(); i++) {
		this->servers.emplace_back(ports[i].port);
		auto& server = this->servers.back();
		server.on_connect += std::bind(&request_server::on_client_connect, this, placeholders::_1);
	}
}

request_server::request_server(request_server&& other) : incoming(0) , outgoing(0) {
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
	this->running = false;
	this->servers = move(other.servers);
	this->incoming = move(other.incoming);
	this->outgoing = move(other.outgoing);

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

	this->running = true;

	this->incoming.on_item += std::bind(&request_server::on_incoming, this, placeholders::_1, placeholders::_2);
	this->outgoing.on_item += std::bind(&request_server::on_outgoing, this, placeholders::_1, placeholders::_2);

	this->incoming.start();
	this->outgoing.start();
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
	this->incoming.stop();
	this->outgoing.stop();
}

tcp_connection& request_server::adopt(tcp_connection&& connection, bool call_on_connect) {
	unique_lock<recursive_mutex> lck(this->client_lock);

	this->clients.push_back(move(connection));
	auto& ref = this->clients.back();

	if (call_on_connect)
		this->on_connect(ref);

	return ref;
}

void request_server::on_client_connect(tcp_connection connection) {
	unique_lock<recursive_mutex> lck(this->client_lock);
	this->clients.push_back(move(connection));
	this->on_connect(this->clients.back());
}

void request_server::on_client_disconnect(tcp_connection& connection) {
	unique_lock<recursive_mutex> lck(this->client_lock);
	auto iter = find_if(this->clients.begin(), this->clients.end(), [&connection](tcp_connection& conn) { return &conn == &connection; });
	this->clients.erase(iter);
}

void request_server::on_incoming(word worker_number, message& request) {
	if (request.data.size() < 4)
		return;

	uint16 id;
	uint8 category, method;
	request.data >> id >> category >> method;

	message response(request.connection, id);

	switch (this->on_request(request.connection, worker_number, category, method, request.data, response.data)) {
		case request_result::success:
			this->enqueue_outgoing(move(response));

			break;
		case request_result::retry_later:
			if (++request.attempts >= request_server::max_retries)
				this->enqueue_incoming(move(request));
			else
				response.data.write(this->retry_code);

			break;
		case request_result::no_response:
			break;
	}
}

void request_server::on_outgoing(word worker_number, message& response) {
	response.connection.send(response.data.data(), response.data.size());
}

void request_server::io_run() {
	while (this->running) {
		unique_lock<recursive_mutex> lck(this->client_lock);

		for (auto& i : this->clients) {
			if (i.data_available()) {
				for (auto& k : i.read()) {
					if (!k.closed) {
						this->enqueue_incoming(message(i, move(k)));
					}
					else {
						this->on_client_disconnect(i);
						goto end;
					}
				}
			}
		}
	end:
		this_thread::sleep_for(chrono::microseconds(500));
	}
}

void request_server::enqueue_incoming(message m) {
	if (!this->running)
		return;

	m.data.seek(0);
	this->incoming.add_work(move(m));
}

void request_server::enqueue_outgoing(message m) {
	if (!this->running)
		return;

	m.data.seek(0);
	this->outgoing.add_work(move(m));
}

request_server::message::message(tcp_connection& connection, tcp_connection::message message) : connection(connection), data(message.data, message.length) {
	this->attempts = 0;
}

request_server::message::message(tcp_connection& connection, data_stream data) : connection(connection), data(move(data)) {
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