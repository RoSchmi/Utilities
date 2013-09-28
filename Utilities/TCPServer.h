#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"

#include <thread>
#include <atomic>
#include <string>

namespace Utilities {
	namespace Net {
		class TCPServer {
			public:
				typedef void (*OnConnectCallback)(TCPConnection&& client, void* state);
			
				exported TCPServer();
				exported TCPServer(std::string port, OnConnectCallback connectCallback, void* onConnectState = nullptr, bool isWebSocket = false);
				exported TCPServer(TCPServer&& other);
				exported TCPServer& operator=(TCPServer&& other);
				exported ~TCPServer();

				exported void start();
				exported void stop();

				TCPServer(const TCPServer& other) = delete;
				TCPServer& operator=(const TCPServer& other) = delete;

			private:
				Socket listener;
				std::string port;
				std::thread acceptWorker;
				std::atomic<bool> active;
				std::atomic<bool> valid;
				bool isWebSocket;
				void* state;
				OnConnectCallback connectCallback;

				void acceptWorkerRun();
		};
	}
}
