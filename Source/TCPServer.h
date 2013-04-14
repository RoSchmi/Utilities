#pragma once

#include "Common.h"
#include "Socket.h"
#include "TCPConnection.h"
#include <thread>
#include <mutex>
#include <vector>
#include <string>

// todo: doc this
namespace Utilities {
	namespace Net {
		class TCPServer {
			public:
				typedef void* (*OnConnectCallback)(TCPConnection& client, void* state, const uint8 clientAddress[Socket::ADDRESS_LENGTH]); /* return a pointer to a state object passed in to OnReceive */
				typedef void (*OnReceiveCallback)(TCPConnection& client, void* state, TCPConnection::Message& message);
			
				exported TCPServer(std::string port, bool isWebSocket, void* onConnectState, OnConnectCallback connectCallback, OnReceiveCallback receiveCallback);
				exported ~TCPServer();

			private:
				Socket listener;
				SocketAsyncWorker asyncWorker;
				std::thread acceptWorker;
				std::mutex clientListLock;
				std::vector<TCPConnection*> clientList;
				bool isWebSocket;
				bool active; //used only for acceptWorker 
				void* state;
				OnConnectCallback connectCallback;
				OnReceiveCallback receiveCallback;

				void acceptWorkerRun();
				void onClientDisconnecting(TCPConnection* client);
				static void asyncReadCallback(const Socket& socket, void* state);

				TCPServer(const TCPServer& other);
				TCPServer& operator=(const TCPServer& other);
				TCPServer(TCPServer&& other);
				TCPServer& operator=(TCPServer&& other);
			
				friend class TCPConnection;
				friend class WebSocketConnection;
		};
	}
}
