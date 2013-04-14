#pragma once

#include "Common.h"
#include "TCPServer.h"
#include "DataStream.h"
#include "SafeQueue.h"

#include <atomic>
#include <vector>
#include <thread>
#include <cstring>

namespace Utilities {
	class RequestServer {
		public: 
			struct Client {
				RequestServer& parent;
				Net::TCPConnection& connection;
				uint8 ipAddress[Utilities::Net::Socket::ADDRESS_LENGTH];
				uint64 authenticatedId;

				Client(Net::TCPConnection& connection, RequestServer& parent, const uint8 clientAddress[Net::Socket::ADDRESS_LENGTH]) : parent(parent), connection(connection) {
					this->authenticatedId = 0;
					memcpy(this->ipAddress, clientAddress, Net::Socket::ADDRESS_LENGTH);
				}
			};
			
			struct Request {
				Client& client;
				DataStream parameters;
				uint8 currentAttempts;

				Request(Client& client, Net::TCPConnection::Message& message) : client(client), parameters(message.data, message.length, false) {
					this->currentAttempts = 0;
				}
			};

			static const uint8 MAX_RETRIES = 5;
			
			typedef bool (*HandlerCallback)(uint8 workerNumber, Client& client, uint8 requestCategory, uint8 requestMethod, DataStream& parameters, DataStream& response, void* state);
			RequestServer(std::string port, uint8 workers, bool usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state = nullptr);
			RequestServer(std::vector<std::string> ports, uint8 workers, std::vector<bool> usesWebSockets, uint16 retryCode, HandlerCallback handler, void* state = nullptr);
			~RequestServer();

		private:
			RequestServer(const RequestServer& other);
			RequestServer(RequestServer&& other);
			RequestServer& operator=(const RequestServer& other);
			RequestServer& operator=(RequestServer&& other);

			std::vector<Net::TCPServer*> servers;
		
			HandlerCallback handler;
			DataStream response;
			SafeQueue<Request> queue;
			uint16 retryCode;
			void* state;

			std::vector<std::thread> workers;
			std::atomic<bool> running;

			void workerRun(uint8 workerNumber);
	
			static void* onClientConnect(Utilities::Net::TCPConnection& connection, void* serverState, const uint8 clientAddress[Utilities::Net::Socket::ADDRESS_LENGTH]);
			static void onRequestReceived(Utilities::Net::TCPConnection& connection, void* state, Utilities::Net::TCPConnection::Message& message);
	};
}
