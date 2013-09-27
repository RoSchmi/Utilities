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
			
				exported TCPServer(std::string port, OnConnectCallback connectCallback, void* onConnectState = nullptr, bool isWebSocket = false);
				exported ~TCPServer();

				exported void close();

				TCPServer(const TCPServer& other) = delete;
				TCPServer& operator=(const TCPServer& other) = delete;
				TCPServer(TCPServer&& other) = delete;
				TCPServer& operator=(TCPServer&& other) = delete;

			private:
				Socket listener;
				std::thread acceptWorker;
				std::atomic<bool> active;
				bool isWebSocket;
				void* state;
				OnConnectCallback connectCallback;

				void acceptWorkerRun();
		};
	}
}
