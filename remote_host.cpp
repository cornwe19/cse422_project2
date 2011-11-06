#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "common.h"
#include "socket_utils.h"

using namespace std;

void MessageReceived( unicast_pkt data, sockaddr_in sender );

int _noisy_ack_port;

int main( int argc, char** argv )
{
	if( argc < 3 )
	{
		cout << "Usage: " << argv[0] << " <num messages> <ack port>" << endl;
		return PROGRAM_FAILURE;
	}

	int expected_num_messages = atoi(argv[1]);
	_noisy_ack_port = atoi( argv[2] );
	
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

	SocketUtils::ReceiveMessages( udp_sock, expected_num_messages, MessageReceived );	

	close( udp_sock );

	return 0;
}

void MessageReceived( unicast_pkt data, sockaddr_in sender )
{
	cout << "<remote_host> received message with contents: " << ntohl( data.data ) << endl; 
	
	char* ip_address = new char[INET_ADDRSTRLEN];
	inet_ntop( AF_INET, &(sender.sin_addr), ip_address, INET_ADDRSTRLEN );

	const char* error = NULL;
	SocketUtils::SendMessage( ip_address, _noisy_ack_port,  data, &error );

	if( error != NULL )
	{
		cerr << "<remote_host> " << error << endl;
		return;
	}

	cout << "<remote_host> ACK sent to " << ip_address << ":" << _noisy_ack_port << endl;

	delete[] ip_address;
}
