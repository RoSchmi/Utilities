#ifndef INCLUDE_UTILITIES_TCPCLIENT
#define INCLUDE_UTILITIES_TCPCLIENT

#include "Common.h"
#include "AsyncList.h"
#include "TCPServer.h"
#include "Common.h"
#include "Thread.h"
#include "Socket.h"

/* forward declarations */
typedef struct TCPClient TCPClient;

typedef void (*TCPClient_OnServerDisconnect)(TCPClient* client, void* state);
typedef void (*TCPClient_OnReceive)(TCPClient* client, void* state, uint8* buffer, uint16 length);

struct TCPClient {
	SAL_Socket* Server;
	TCPClient_OnReceive ReceiveCallback;
	TCPClient_OnServerDisconnect DisconnectCallback;
	uint8 Buffer[MESSAGE_MAXSIZE];
	uint16 BytesReceived;
	void* State;
};

exported TCPClient* TCPClient_Connect(int8* address, int8* port, TCPClient_OnReceive receiveCallback, TCPClient_OnServerDisconnect serverDisconnectCallback, void* state);
exported boolean TCPClient_Send(TCPClient* client, uint8* buffer, uint16 length);
exported void TCPClient_Disconnect(TCPClient* client);

#endif
