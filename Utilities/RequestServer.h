#pragma once

#include "Common.h"
#include "TCPServer.h"
#include "DataStream.h"
#include "SafeQueue.h"

#include <atomic>
#include <vector>
#include <thread>
#include <cstring>
#include <mutex>

namespace Utilities {
	class RequestServer {
		public: 
			struct Client {
				uint64 id;
				RequestServer& parent;
				Net::TCPConnection& connection;
				uint8 ipAddress[Utilities::Net::Socket::ADDRESS_LENGTH];
				uint64 authenticatedId;

				exported Client(Net::TCPConnection& connection, RequestServer& parent, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH], uint64 id) : parent(parent), connection(connection) {
					this->authenticatedId = 0;
					this->id = id;
					memcpy(this->ipAddress, clientAddress, Net::Socket::ADDRESS_LENGTH);
				}
			};
			
			struct Message {
				Client& client;
				DataStream data;
				uint8 currentAttempts;

				exported Message(Client& client, Net::TCPConnection::Message& message) : client(client), data(message.data, message.length, false) {
					this->currentAttempts = 0;
				}

				exported Message(Client& client, DataStream& message) : client(client), data(std::move(message)) {
					this->currentAttempts = 0;
				}
			};

			static const uint8 MAX_RETRIES = 5;
			
			typedef bool (*HandlerCallback)(uint8 workerNumber, Client& client, uint8 requestCategory, uint8 requestMethod, DataStream& parameters, DataStream& response, void* state);
			exported RequestServer(std::string port, uint8 workers, bool usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state = nullptr);
			exported RequestServer(std::vector<std::string> ports, uint8 workers, std::vector<bool> usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state = nullptr);
			exported ~RequestServer();

			void send(uint64 authenticatedId, uint64 sourceConnectionId, DataStream& message);

		private:
			RequestServer(const RequestServer& other);
			RequestServer(RequestServer&& other);
			RequestServer& operator=(const RequestServer& other);
			RequestServer& operator=(RequestServer&& other);

			std::vector<Net::TCPServer*> servers;
			std::map<uint64, std::map<uint64, Client*>> clients;
			std::mutex clientListLock;
		
			HandlerCallback handler;
			DataStream response;
			SafeQueue<Message> queue;
			SafeQueue<Message> outgoingQueue;
			uint16 retryCode;
			void* state;
			uint64 nextId;

			std::thread outgoingWorker;
			std::vector<std::thread> workers;
			std::atomic<bool> running;
			
			void workerRun(uint8 workerNumber);
			void outgoingWorkerRun();
	
			static void* onClientConnect(Utilities::Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Utilities::Net::Socket::ADDRESS_LENGTH]);
			static void onRequestReceived(Utilities::Net::TCPConnection& connection, void* state, Utilities::Net::TCPConnection::Message& message);
	};
}
