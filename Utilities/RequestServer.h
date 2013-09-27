#pragma once

#include "Common.h"
#include "TCPServer.h"
#include "DataStream.h"

#include <atomic>
#include <vector>
#include <list>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Utilities {
	namespace Net {
		class RequestServer {
			public:
				struct exported Message {
					Net::TCPConnection& connection;
					word currentAttempts;
					DataStream data;

					Message(Net::TCPConnection& connection, const uint8* data, word length);
					Message(Net::TCPConnection& connection, uint16 id, uint8 category = 0, uint8 method = 0);
					Message(Message&& other);

					static void writeHeader(DataStream& stream, uint16 id, uint8 category, uint8 method);

					Message(const Message& other) = delete;
					Message& operator=(const Message& other) = delete;
					Message& operator=(Message&& other) = delete;
				};

				static const word MAX_RETRIES = 5;

				typedef bool(*RequestCallback)(Net::TCPConnection& connection, void* state, word workerNumber, uint8 requestCategory, uint8 requestMethod, DataStream& parameters, DataStream& response);
				typedef void(*ConnectCallback)(Net::TCPConnection& connection, void* state);
				typedef void(*DisconnectCallback)(Net::TCPConnection& connection, void* state);

				exported RequestServer(std::string port, bool usesWebSockets, word workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
				exported RequestServer(std::vector<std::string> ports, std::vector<bool> usesWebSockets, word workers, uint16 retryCode, RequestCallback onRequest, ConnectCallback onConnect, DisconnectCallback onDisconnect, void* state = nullptr);
				exported ~RequestServer();

				exported void addToIncomingQueue(Message&& message);
				exported void addToOutgoingQueue(Message&& message);

				RequestServer(const RequestServer& other) = delete;
				RequestServer& operator=(const RequestServer& other) = delete;
				RequestServer(RequestServer&& other) = delete;
				RequestServer& operator=(RequestServer&& other) = delete;

			private:
				std::list<Net::TCPServer> servers;
				std::vector<Net::TCPConnection> clients;
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

				void incomingWorkerRun(word workerNumber);
				void outgoingWorkerRun();
				void ioWorkerRun();

				static void onClientConnect(Net::TCPConnection&& connection, void* serverState);
		};
	}
}
