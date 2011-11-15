#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>

#include "common.h"
#include "socket_utils.h"

using namespace std;

void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender, struct timeval** timeout );
void ConsoleReceived( const char* message, bool* should_exit );

int _noisy_ack_port;
int _num_messages_received;
unicast_pkt _last_message_received;

int main( int argc, char** argv )
{
	if( argc < 2 )
	{
		cout << "Usage: " << argv[0] << " <ack port>" << endl;
		return PROGRAM_FAILURE;
	}

	_noisy_ack_port = atoi( argv[1] );
	_num_messages_received = 0;
	
	// Bind UDP socket
	int udp_port = -1;
	const char* udp_error = NULL;
   int udp_sock = SocketUtils::BindSocket( SOCK_DGRAM, udp_port, &udp_error );
   if( udp_error != NULL )
   {
      cerr << udp_error << endl;
      return PROGRAM_FAILURE;
   }
   else
   {
      cout << "UDP server running on port: " << udp_port << endl;
   }

	SocketUtils::ReceiveMessages( &udp_sock, 1, SocketReceived, ConsoleReceived );	

	close( udp_sock );

	return 0;
}

void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender, struct timeval** timeout )
{
	cout << "<remote_host> received message with contents: " << ntohl( data.data ) << endl; 
	
	if( _last_message_received.data < data.data )
	{
	   _last_message_received = data;
	   _num_messages_received++;
	}

	char* ip_address = new char[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &(sender.sin_addr), ip_address, INET_ADDRSTRLEN );

	const char* error = NULL;
	int message_sock = SocketUtils::SendMessage( ip_address, _noisy_ack_port,  data, &error );
	close( message_sock );

	if( error != NULL )
	{
		cerr << "<remote_host> " << error << endl;
		return;
	}

	cout << "<remote_host> ACK sent to " << ip_address << ":" << _noisy_ack_port << endl;

	delete[] ip_address;
}

void ConsoleReceived( const char* message, bool* should_exit )
{
	if( strncmp( message, "q", 1 ) == 0 || strncmp( message, "Q", 1 ) == 0 )
	{
		*should_exit = true;
		cout << "Received " << _num_messages_received << " messages from base station" << endl;
	}
}
