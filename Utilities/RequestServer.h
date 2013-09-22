#pragma once

#include "Common.h"
#include "TCPServer.h"
#include "DataStream.h"

#include <atomic>
#include <vector>
#include <queue>
#include <thread>
#include <cstring>
#include <mutex>
#include <forward_list>
#include <condition_variable>
#include <memory>

namespace Utilities {
	class RequestServer {
		public: 
			struct Message {
				Net::TCPConnection& connection;
				uint8 currentAttempts;
				DataStream data;

				exported Message(Net::TCPConnection& connection, const uint8* data, word length);
				exported Message(Net::TCPConnection& connection, uint16 id, uint8 category = 0, uint8 method = 0);

				exported static void writeHeader(DataStream& stream, uint16 id, uint8 category, uint8 method);
			};

			static const uint8 MAX_RETRIES = 5;
			
			typedef bool(*RequestCallback)(Net::TCPConnection& connection, void* state, uint8 workerNumber, uint8 requestCategory, uint8 requestMethod, DataStream& parameters, DataStream& response);
			typedef void(*ConnectCallback)(Net::TCPConnection& connection, void* state);
			typedef void(*DisconnectCallback)(Net::TCPConnection& connection, void* state);

			exported RequestServer(std::string port, bool usesWebSockets, uint8 workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
			exported RequestServer(std::vector<std::string> ports, std::vector<bool> usesWebSockets, uint8 workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
			exported ~RequestServer();

			exported void addToIncomingQueue(Message message);
			exported void addToOutgoingQueue(Message message);

			RequestServer() = delete;
			RequestServer(const RequestServer& other) = delete;
			RequestServer& operator=(const RequestServer& other) = delete;
			RequestServer(RequestServer&& other) = delete;
			RequestServer& operator=(RequestServer&& other) = delete;

	    private:
			Net::TCPServer server;
			std::forward_list<Net::TCPConnection> clients;
			std::mutex clientListLock;
		
			RequestCallback onRequest;
			ConnectCallback onConnect;
			DisconnectCallback onDisconnect;

			std::queue<Message> incomingQueue;
			std::queue<Message> outgoingQueue;
			std::mutex incomingLock;
			std::mutex outgoingLock;
			std::condition_variable incomingCV;
			std::condition_variable outgoingCV;

			uint16 retryCode;
			void* state;

			std::thread ioWorker;
			std::thread outgoingWorker;
			std::vector<std::thread> incomingWorkers;
			std::atomic<bool> running;
			
			void incomingWorkerRun(uint8 workerNumber);
			void outgoingWorkerRun();
			void ioWorkerRun();
	
			static void onClientConnect(Utilities::Net::TCPConnection connection, void* serverState);
	};
}
