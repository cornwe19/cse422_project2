#pragma once

#include "common.h"

typedef void (*ReceiveCallback)( unicast_pkt data, sockaddr_in sender ); 

class SocketUtils
{
public:
	static int BindSocket( int type, int& port, const char** error );
	static void ReceiveMessages( int socket, int numMessages, ReceiveCallback messageReceived ); 
};

