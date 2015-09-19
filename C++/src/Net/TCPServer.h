#pragma once

#include <thread>
#include <atomic>
#include <string>
#include <memory>

#include "../Common.h"
#include "../Event.h"
#include "Socket.h"
#include "TCPConnection.h"

namespace util {
	namespace net {
		class tcp_server {
			public:
				tcp_server();
				tcp_server(endpoint ep);
				tcp_server(tcp_server&& other);
				tcp_server& operator=(tcp_server&& other);
				~tcp_server();

				void start();
				void stop();

				class cant_move_running_server_exception {};
				class cant_start_default_constructed_exception {};

				tcp_server(const tcp_server& other) = delete;
				tcp_server& operator=(const tcp_server& other) = delete;

#ifdef WINDOWS
				event_single<void, std::unique_ptr<tcp_connection>, void*> on_connect;
				void* state;
#else
				event_single<void, std::unique_ptr<tcp_connection>> on_connect;
#endif
			private:
				socket listener;
				endpoint ep;
				std::thread accept_worker;
				std::atomic<bool> active;
				std::atomic<bool> valid;

				void accept_worker_run();
		};
	}
}
