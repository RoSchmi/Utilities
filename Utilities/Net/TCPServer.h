#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"

#include <thread>
#include <atomic>
#include <string>
#include <functional>

namespace util {
	namespace net {
		class tcp_server {
			public:
				typedef std::function<void(tcp_connection&& client, void* state)> on_connect_callback;
			
				exported tcp_server();
				exported tcp_server(std::string port, on_connect_callback on_connect, void* on_connect_state = nullptr, bool is_websocket = false);
				exported tcp_server(tcp_server&& other);
				exported tcp_server& operator=(tcp_server&& other);
				exported ~tcp_server();

				exported void start();
				exported void stop();

				class cant_move_running_server_exception {};
				class cant_start_default_constructed_exception {};

				tcp_server(const tcp_server& other) = delete;
				tcp_server& operator=(const tcp_server& other) = delete;

			private:
				socket listener;
				std::string port;
				std::thread accept_worker;
				std::atomic<bool> active;
				std::atomic<bool> valid;
				bool is_websocket;
				void* state;
				on_connect_callback on_connect;

				void accept_workerRun();
		};
	}
}
