#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>

#include "common.h"
#include "socket_utils.h"

using namespace std;

void MessageReceived( unicast_pkt data, sockaddr_in sender );

int main( int argc, char** argv )
{
	// TODO: make this run on a timer instead of relying on an expected message count
	int expected_num_messages = atoi(argv[1]);
	
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
	cout << "<base_station> received message with contents: " << ntohl( data.data ) << endl; 
}
