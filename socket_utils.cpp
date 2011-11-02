#include <sys/socket.h>
#include <netinet/in.h>

#include "common.h"
#include "socket_utils.h"

int SocketUtils::BindSocket( int type, int& port, const char** error )
{
   int sock = socket( AF_INET, type, DEFAULT_PROTOCOL );
   if( sock < 0 )
   {
      *error = "Failed to create UDP socket.";
      return sock;
   }

   sockaddr_in server;
   socklen_t server_length = sizeof(server);

   const int ASSIGN_DEFAULT_PORT = 0;
   server.sin_family = AF_INET;
   server.sin_port = ASSIGN_DEFAULT_PORT;
   server.sin_addr.s_addr = INADDR_ANY;

   if( bind( sock, (sockaddr*)&server, server_length ) < 0 )
   {
      *error = "Failed to bind UDP socket.";
      return sock;
   }
   
   getsockname( sock, (sockaddr*)&server, &server_length );
   // Convert back from big endian (ntohs)
   port = ntohs(server.sin_port);

   return sock;
}

void SocketUtils::ReceiveMessages( int socket, int numMessages, ReceiveCallback messageReceived )
{
	sockaddr_in client;
   socklen_t client_length = sizeof(client);
   unicast_pkt sensor_data;
	
	int num_messages_received = 0;
	while( num_messages_received < numMessages )
	{
   	recvfrom( socket, &sensor_data,
      	sizeof(sensor_data), 0, (sockaddr*)&client, &client_length );
		
		(*messageReceived)( sensor_data, client );

      num_messages_received++;
	}
}

