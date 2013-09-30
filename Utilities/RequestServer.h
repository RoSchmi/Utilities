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

namespace util {
	namespace net {
		class request_server {
			public:
				struct exported message {
					tcp_connection& connection;
					word currentAttempts;
					data_stream data;

					message(tcp_connection& connection, tcp_connection::message&& message);
					message(tcp_connection& connection, data_stream&& data);
					message(tcp_connection& connection, const uint8* data, word length);
					message(tcp_connection& connection, uint16 id, uint8 category = 0, uint8 method = 0);
					message(message&& other);

					static void writeHeader(data_stream& stream, uint16 id, uint8 category, uint8 method);

					message(const message& other) = delete;
					message& operator=(const message& other) = delete;
					message& operator=(message&& other) = delete;
				};

				enum class request_result {
					SUCCESS,
					NO_RESPONSE,
					RETRY_LATER
				};

				static const word MAX_RETRIES = 5;

				typedef request_result(*on_request_callback)(tcp_connection& connection, void* state, word workerNumber, uint8 requestCategory, uint8 requestMethod, data_stream& parameters, data_stream& response);
				typedef void(*on_connect_callback)(tcp_connection& connection, void* state);
				typedef void(*on_disconnect_callback)(tcp_connection& connection, void* state);

				exported request_server();
				exported request_server(std::string port, bool usesWebSockets, word workers, uint16 retryCode, on_request_callback onRequest, on_connect_callback onConnect, on_disconnect_callback onDisconnect, void* state = nullptr);
				exported request_server(std::vector<std::string> ports, std::vector<bool> usesWebSockets, word workers, uint16 retryCode, on_request_callback onRequest, on_connect_callback onConnect, on_disconnect_callback onDisconnect, void* state = nullptr);
				exported request_server(request_server&& other);
				exported ~request_server();
				exported request_server& operator=(request_server&& other); 

				exported void addToIncomingQueue(message&& message);
				exported void addToOutgoingQueue(message&& message);

				exported void start();
				exported void stop();
				exported tcp_connection& adoptConnection(tcp_connection&& connection, bool callOnClientConnect = false);

				request_server(const request_server& other) = delete;
				request_server& operator=(const request_server& other) = delete;

			private:
				std::list<tcp_server> servers;
				std::vector<tcp_connection> clients;
				std::mutex clientListLock;

				on_request_callback onRequest;
				on_connect_callback onConnect;
				on_disconnect_callback onDisconnect;

				std::queue<message> incomingQueue;
				std::queue<message> outgoingQueue;
				std::mutex incomingLock;
				std::mutex outgoingLock;
				std::condition_variable incomingCV;
				std::condition_variable outgoingCV;

				uint16 retryCode;
				void* state;
				word workers;

				std::thread ioWorker;
				std::thread outgoingWorker;
				std::vector<std::thread> incomingWorkers;
				std::atomic<bool> running;
				std::atomic<bool> valid;

				void incomingWorkerRun(word workerNumber);
				void outgoingWorkerRun();
				void ioWorkerRun();

				static void onClientConnect(tcp_connection&& connection, void* serverState);
		};
	}
}
