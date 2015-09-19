#pragma once

#include <atomic>
#include <vector>
#include <list>
#include <thread>
#include <memory>

#include "../Common.h"
#include "../DataStream.h"
#include "../WorkProcessor.h"
#include "../Event.h"
#include "TCPServer.h"
#include "TCPConnection.h"

namespace util {
	namespace net {
		class request_server {
			public:
				struct message {
					std::shared_ptr<tcp_connection> connection;
					word attempts;
					data_stream data;

					message(std::shared_ptr<tcp_connection> connection, tcp_connection::message message);
					message(std::shared_ptr<tcp_connection> connection, data_stream data);
					message(std::shared_ptr<tcp_connection> connection, const uint8* data, word length);
					message(std::shared_ptr<tcp_connection> connection, uint16 id, uint8 category = 0, uint8 method = 0);
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

				static const word max_retries = 5;

				request_server();
				request_server(net::endpoint port, word workers, uint16 retry_code);
				request_server(std::vector<net::endpoint> ports, word workers, uint16 retry_code);
				request_server(request_server&& other);
				~request_server();

				request_server& operator=(request_server&& other); 

				void enqueue_incoming(message message);
				void enqueue_outgoing(message message);

				void start();
				void stop();
				std::shared_ptr<tcp_connection> adopt(tcp_connection&& connection, bool call_on_connect = false);
				
				request_server(const request_server& other) = delete;
				request_server& operator=(const request_server& other) = delete;

				event_single<request_result, std::shared_ptr<tcp_connection>, word, uint8, uint8, data_stream&, data_stream&> on_request;
				event<std::shared_ptr<tcp_connection>> on_connect;
				event<std::shared_ptr<tcp_connection>> on_disconnect;

			private:
				std::list<tcp_server> servers;
				std::vector<std::shared_ptr<tcp_connection>> clients;
				std::recursive_mutex client_lock;

				work_processor<message> incoming;
				work_processor<message> outgoing;
				async_worker io_worker;

				uint16 retry_code;

				std::atomic<bool> running;
				std::atomic<bool> valid;

				void on_client_connect(std::unique_ptr<tcp_connection> connection);
				void on_client_disconnect(std::shared_ptr<tcp_connection> connection);
				void on_incoming(word worker_number, message& response);
				void on_outgoing(word worker_number, message& response);
				bool on_data(std::shared_ptr<tcp_connection> connection);

#ifdef WINDOWS
				static void on_client_connect_hack(std::unique_ptr<tcp_connection> connection, void* state);
#endif
		};
	}
}
