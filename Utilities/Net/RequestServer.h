#pragma once

#include <atomic>
#include <vector>
#include <list>
#include <thread>
#include <functional>

#include "../Common.h"
#include "../DataStream.h"
#include "../WorkQueue.h"
#include "TCPServer.h"

namespace util {
	namespace net {
		class request_server {
			public:
				struct exported message {
					tcp_connection& connection;
					word attempts;
					data_stream data;

					message(tcp_connection& connection, tcp_connection::message&& message);
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
					SUCCESS,
					NO_RESPONSE,
					RETRY_LATER
				};

				class cant_move_running_server_exception {};
				class cant_start_default_constructed_exception {};

				static const word MAX_RETRIES = 5;

				typedef std::function<request_result(tcp_connection& connection, void* state, word worker_number, uint8 category, uint8 method, data_stream& parameters, data_stream& response)> on_request_callback;
				typedef std::function<void(tcp_connection& connection, void* state)> on_connect_callback;
				typedef std::function<void(tcp_connection& connection, void* state)> on_disconnect_callback;

				exported request_server();
				exported request_server(std::string port, bool uses_websockets, word workers, uint16 retry_code, on_request_callback on_request, on_connect_callback on_connect, on_disconnect_callback on_disconnect, void* state = nullptr);
				exported request_server(std::vector<std::string> ports, std::vector<bool> uses_websockets, word workers, uint16 retry_code, on_request_callback on_request, on_connect_callback on_connect, on_disconnect_callback on_disconnect, void* state = nullptr);
				exported request_server(request_server&& other);
				exported ~request_server();

				exported request_server& operator=(request_server&& other); 

				exported void enqueue_incoming(message&& message);
				exported void enqueue_outgoing(message&& message);

				exported void start();
				exported void stop();
				exported tcp_connection& adopt(tcp_connection&& connection, bool call_on_connect = false);

				request_server(const request_server& other) = delete;
				request_server& operator=(const request_server& other) = delete;

			private:
				std::list<tcp_server> servers;
				std::vector<tcp_connection> clients;
				std::mutex client_lock;

				on_request_callback on_request;
				on_connect_callback on_connect;
				on_disconnect_callback on_disconnect;

				work_queue<message> incoming;
				work_queue<message> outgoing;

				uint16 retry_code;
				void* state;
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
