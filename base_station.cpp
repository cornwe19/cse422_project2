#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "common.h"
#include "socket_utils.h"

using namespace std;

int _noisy_link_port;
char* _noisy_link_IP;
int _noisy_link_sock;
int _sensor_sock;

void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender );
void ConsoleReceived( const char* message, bool* should_exit );

int main( int argc, char** argv )
{
	if( argc < 3 )
	{
		 cout << "Usage: " << argv[0] << " <noisy link IP> <noisy link port>" << endl;
		 return PROGRAM_FAILURE;
	}

	_noisy_link_IP = argv[1]; 
	_noisy_link_port = atoi( argv[2] );

	_noisy_link_sock = socket( AF_INET, SOCK_DGRAM, DEFAULT_PROTOCOL );
	if( _noisy_link_sock < 0 )
	{
		cerr << "<base_station> " << "Failed to open noisy link socket." << endl;
		return PROGRAM_FAILURE;
	}

	// Bind UDP socket
	int udp_port = -1;
	const char* udp_error = NULL;
   _sensor_sock = SocketUtils::BindSocket( SOCK_DGRAM, udp_port, &udp_error );
   if( udp_error != NULL )
   {
      cerr << udp_error << endl;
      return PROGRAM_FAILURE;
   }
   else
   {
      cout << "UDP server running on port: " << udp_port << endl;
   }

	int sockets[] = { _sensor_sock, _noisy_link_sock };

	SocketUtils::ReceiveMessages( sockets, 2, SocketReceived, ConsoleReceived );	

	close( _sensor_sock );
	close( _noisy_link_sock );

	return 0;
}

void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender )
{
	cout << "<base_station> received message with contents: " << ntohl( data.data ) << endl;

	if ( _noisy_link_sock == socket )
	{
		cout << "<base_station> received ack for " << ntohl( data.data ) << endl;
	}
	else if ( _sensor_sock == socket )
	{
		SocketUtils::SendMessage( _noisy_link_IP, _noisy_link_port, data, _noisy_link_sock );

		cout << "<base_station> Forwarded message " << ntohl( data.data ) << " to "
		  	  << _noisy_link_IP << ":" << _noisy_link_port << endl;
	}
}

void ConsoleReceived( const char* message, bool* should_exit )
{
	if( strncmp( message, "q", 1 ) == 0 || strncmp( message, "Q", 1 ) == 0 )
	{
		*should_exit = true;
	}
}

