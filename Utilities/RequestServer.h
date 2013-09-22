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
#include <condition_variable>

namespace Utilities {
	class RequestServer {
		public: 
			struct Client {
				uint64 id;
				RequestServer& parent;
				Net::TCPConnection& connection;
				uint8 ipAddress[Net::Socket::ADDRESS_LENGTH];
				void* state;

				exported Client(Net::TCPConnection& connection, RequestServer& parent, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]);
			};
			
			struct Message {
				Client& client;
				DataStream data;
				uint8 currentAttempts;

				exported Message(Client& client, const DataStream& message);
				exported Message(Client& client, uint16 id, uint8 category = 0, uint8 method = 0);

				exported static void getHeader(DataStream& stream, uint16 id, uint8 category, uint8 method);
			};

			static const uint8 MAX_RETRIES = 5;
			
			typedef bool (*RequestCallback)(uint8 workerNumber, Client& client, uint8 requestCategory, uint8 requestMethod, DataStream& parameters, DataStream& response, void* state);
			typedef void* (*ConnectCallback)(Client& client, void* state);
			typedef void (*DisconnectCallback)(Client& client, void* state);

			exported RequestServer();
			exported RequestServer(std::string port, uint8 workers, bool usesWebSockets, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
			exported RequestServer(std::vector<std::string> ports, uint8 workers, std::vector<bool> usesWebSockets, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
			exported ~RequestServer();

			exported void addToIncomingQueue(Message* message);
			exported void addToOutgoingQueue(Message* message);

		private:
			RequestServer(const RequestServer& other);
			RequestServer& operator=(const RequestServer& other);
			RequestServer(RequestServer&& other);
			RequestServer& operator=(RequestServer&& other);

			std::vector<Net::TCPServer*> servers;
			std::map<uint64, Client*> clients;
			std::mutex clientListLock;
		
			RequestCallback onRequest;
			ConnectCallback onConnect;
			DisconnectCallback onDisconnect;

			std::queue<Message*> incomingQueue;
			std::queue<Message*> outgoingQueue;
			std::mutex incomingLock;
			std::mutex outgoingLock;
			std::condition_variable incomingCV;
			std::condition_variable outgoingCV;

			uint16 retryCode;
			void* state;
			uint64 nextId;

			std::thread outgoingWorker;
			std::vector<std::thread> incomingWorkers;
			std::atomic<bool> running;
			
			void incomingWorkerRun(uint8 workerNumber);
			void outgoingWorkerRun();
			void shutdown();
	
			static void* onClientConnect(Utilities::Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Utilities::Net::Socket::ADDRESS_LENGTH]);
			static void onRequestReceived(Utilities::Net::TCPConnection& connection, void* state, Utilities::Net::TCPConnection::Message& message);
	};
}
