#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <iostream>
#include <algorithm>

#include "socket_utils.h"

using namespace std;

int SocketUtils::BindSocket( int type, int& port, const char** error )
{
   int sock = socket( AF_INET, type, DEFAULT_PROTOCOL );
   if( sock < 0 )
   {
      *error = "Failed to create UDP socket.";
      return sock;
   }
	
	int sock_reuse_opt = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse_opt, sizeof(sock_reuse_opt));
   
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

void SocketUtils::ReceiveMessages( int* descriptors, int descriptors_len,
                                   SocketCallback socketMessageReceived,
                                   ConsoleCallback consoleMessageReceived,
                                   TimeoutCallback timeoutFired )
{	
	int fd_max = 0, offset_for_fd_upper_bound = 1;
	for ( int fd = 0; fd < descriptors_len; fd++ )
	{
		fd_max = max( fd_max, descriptors[fd] + offset_for_fd_upper_bound );
	}

   fd_set readset;
	FD_ZERO( &readset );

	sockaddr_in client;
   socklen_t client_length = sizeof(client);
   unicast_pkt sensor_data;

   timeval* timeout = NULL;

   bool should_exit = false;
	while( !should_exit )
	{
		for( int fd_index = 0; fd_index < descriptors_len; fd_index++ )
		{
			FD_SET( descriptors[fd_index], &readset );
		}
		FD_SET( 0, &readset );

      if( select( fd_max, &readset, NULL, NULL, timeout ) > 0 )
      {
         if( timeout != NULL )
         {
            //delete timeout;
            timeout = NULL;
         }

         for( int fd = 0; fd < descriptors_len; fd++ )
         {
            if( FD_ISSET( descriptors[fd], &readset ) )
            {
               recvfrom( descriptors[fd], &sensor_data,
                  sizeof(sensor_data), 0, (sockaddr*)&client, &client_length );

               (*socketMessageReceived)( descriptors[fd], sensor_data, client, &timeout );

               FD_CLR( descriptors[fd], &readset );
            }
         }

         const int console_fd = 0;
         if( FD_ISSET( console_fd, &readset ) )
         {
            char message[256];
            cin >> message;
            (*consoleMessageReceived)( message, &should_exit );
         }
      }
      else if( timeoutFired != NULL )
      {
         cout << "Timer expired, retransmitting message" << endl;
         (*timeoutFired)();
      }
	}
}

int SocketUtils::SendMessage( const char* dest_ip, int dest_port, unicast_pkt data, const char** error )
{
	int dest_sock = socket( AF_INET, SOCK_DGRAM, DEFAULT_PROTOCOL );
	if( dest_sock < 0 )
	{
		*error = "Failed to create socket for noisy link sending";
		return -1;
	}

	SendMessage( dest_ip, dest_port, data, dest_sock );
	
	return dest_sock;
}

void SocketUtils::SendMessage( const char* dest_ip, int dest_port, unicast_pkt data, int dest_sock )
{
	sockaddr_in dest_addr;

	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons( dest_port );
	inet_pton( AF_INET, dest_ip, &dest_addr.sin_addr );

	sendto( dest_sock, &data, sizeof(data), 0, (struct sockaddr*) &dest_addr, sizeof(dest_addr) );
}

