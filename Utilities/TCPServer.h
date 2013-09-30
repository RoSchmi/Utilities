#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"

#include <thread>
#include <atomic>
#include <string>

namespace util {
	namespace net {
		class tcp_server {
			public:
				typedef void (*on_connect_callback)(tcp_connection&& client, void* state);
			
				exported tcp_server();
				exported tcp_server(std::string port, on_connect_callback connectCallback, void* onConnectState = nullptr, bool isWebSocket = false);
				exported tcp_server(tcp_server&& other);
				exported tcp_server& operator=(tcp_server&& other);
				exported ~tcp_server();

				exported void start();
				exported void stop();

				tcp_server(const tcp_server& other) = delete;
				tcp_server& operator=(const tcp_server& other) = delete;

			private:
				socket listener;
				std::string port;
				std::thread acceptWorker;
				std::atomic<bool> active;
				std::atomic<bool> valid;
				bool isWebSocket;
				void* state;
				on_connect_callback connectCallback;

				void acceptWorkerRun();
		};
	}
}
