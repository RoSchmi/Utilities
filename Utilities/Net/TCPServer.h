#pragma once

#include <thread>
#include <atomic>
#include <string>

#include "../Common.h"
#include "../Event.h"
#include "Socket.h"
#include "TCPConnection.h"

namespace util {
	namespace net {
		class tcp_server {
			public:
				exported tcp_server();
				exported tcp_server(std::string port, bool is_websocket = false);
				exported tcp_server(tcp_server&& other);
				exported tcp_server& operator=(tcp_server&& other);
				exported ~tcp_server();

				exported void start();
				exported void stop();

				class cant_move_running_server_exception {};
				class cant_start_default_constructed_exception {};

				tcp_server(const tcp_server& other) = delete;
				tcp_server& operator=(const tcp_server& other) = delete;

				event_single<tcp_server, void, tcp_connection&&, void*> on_connect;
				void* state;

			private:
				socket listener;
				std::string port;
				std::thread accept_worker;
				std::atomic<bool> active;
				std::atomic<bool> valid;
				bool is_websocket;

				void accept_worker_run();
		};
	}
}
