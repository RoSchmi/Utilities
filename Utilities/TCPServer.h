#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <memory>
#include <string>

namespace Utilities {
	namespace Net {
		class TCPServer {
			public:
				typedef void (*OnConnectCallback)(TCPConnection client, void* state);
			
				exported TCPServer(std::string port, bool isWebSocket, OnConnectCallback connectCallback, void* onConnectState = nullptr);
				exported ~TCPServer();

			private:
				Socket listener;
				std::thread acceptWorker;
				bool isWebSocket;
				std::atomic<bool> active;
				void* state;
				OnConnectCallback connectCallback;

				void acceptWorkerRun();

				TCPServer();
				TCPServer(const TCPServer& other);
				TCPServer& operator=(const TCPServer& other);
				TCPServer(TCPServer&& other);
				TCPServer& operator=(TCPServer&& other);
		};
	}
}
