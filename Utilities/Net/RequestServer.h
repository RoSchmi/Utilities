#pragma once

#include <atomic>
#include <vector>
#include <list>
#include <thread>

#include "../Common.h"
#include "../DataStream.h"
#include "../WorkQueue.h"
#include "../Event.h"
#include "TCPServer.h"

namespace util {
	namespace net {
		class request_server {
			public:
				struct exported message {
					tcp_connection& connection;
					word attempts;
					data_stream data;

					message(tcp_connection& connection, tcp_connection::message message);
					message(tcp_connection& connection, data_stream&& data);
					message(tcp_connection& connection, const uint8* data, word length);
					message(tcp_connection& connection, uint16 id, uint8 category = 0, uint8 method = 0);
					message(message&& other);

					static void write_header(data_stream& stream, uint16 id, uint8 category, uint8 method);

					message(const message& other) = delete;
					message& operator=(const message& other) = delete;
					message& operator=(message&& other) = delete;
				};

				enum class request_result {
					success,
					no_response,
					retry_later
				};

				class cant_move_running_server_exception {};
				class cant_start_default_constructed_exception {};

				static const word MAX_RETRIES = 5;

				exported request_server();
				exported request_server(std::string port, word workers, uint16 retry_code, bool uses_websockets = false);
				exported request_server(std::vector<std::string> ports, word workers, uint16 retry_code, std::vector<bool> uses_websockets = { });
				exported request_server(request_server&& other);
				exported ~request_server();

				exported request_server& operator=(request_server&& other); 

				exported void enqueue_incoming(message message);
				exported void enqueue_outgoing(message message);

				exported void start();
				exported void stop();
				exported tcp_connection& adopt(tcp_connection&& connection, bool call_on_connect = false);

				request_server(const request_server& other) = delete;
				request_server& operator=(const request_server& other) = delete;

				event_single<request_server, request_result, tcp_connection&, word, uint8, uint8, data_stream&, data_stream&, void*> on_request;
				event<request_server, void, tcp_connection&, void*> on_connect;
				event<request_server, void, tcp_connection&, void*> on_disconnect;
				void* state;

			private:
				std::list<tcp_server> servers;
				std::vector<tcp_connection> clients;
				std::mutex client_lock;

				work_queue<message> incoming;
				work_queue<message> outgoing;

				uint16 retry_code;
				word workers;

				std::thread io_worker;
				std::thread outgoing_worker;
				std::vector<std::thread> incoming_workers;
				std::atomic<bool> running;
				std::atomic<bool> valid;

				void incoming_run(word worker_number);
				void outgoing_run();
				void io_run();

				static void on_client_connect(tcp_connection&& connection, void* serverState);
		};
	}
}
