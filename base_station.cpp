#include <stdlib.h>
#include <arpa/inet.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>

#include "common.h"
#include "socket_utils.h"

using namespace std;

#define RETRANSMIT_USEC 500 // Retransmit every .5 milliseconds

int _noisy_link_port;
char* _noisy_link_IP;
int _noisy_link_sock;
int _sensor_sock;

// keep track of last received message and last received ack
unicast_pkt _current_message;
unicast_pkt _currented_acked_message;

// keep track of number of transmissions
double _num_transmissions;
double _num_messages_total;

void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender, struct timeval** timeout );
void ConsoleReceived( const char* message, bool* should_exit );
void RetransmitMessage(struct timeval** timeout);
void SetTimeOut( struct timeval** timeout );

int main( int argc, char** argv )
{
	if( argc < 3 )
	{
		 cout << "Usage: " << argv[0] << " <noisy link IP> <noisy link port>" << endl;
		 return PROGRAM_FAILURE;
	}

	_noisy_link_IP = argv[1]; 
	_noisy_link_port = atoi( argv[2] );

	_num_transmissions = 0;
	_num_messages_total = 0;

	// Setup socket to noisy link
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
      cout << "<base_station> Server started. Running on port: " << udp_port << " -- press 'q' to exit" << endl;
   }

   // Register noisy link and sensor network as file descriptors to listen to
   // NOTE: sets up a delegate method for timing out when waiting for ACKs
	int sockets[] = { _sensor_sock, _noisy_link_sock };
	SocketUtils::ReceiveMessages( sockets, 2, SocketReceived, ConsoleReceived, RetransmitMessage );

   // Clean up sockets before exiting
	close( _sensor_sock );
	close( _noisy_link_sock );

	return 0;
}

// Called when utils detect data on a socket
void SocketReceived( int socket, unicast_pkt data, sockaddr_in sender, struct timeval** timeout )
{
	// Got an ACK
   if ( _noisy_link_sock == socket )
	{
		cout << "<base_station> Received ACK for " << ntohl( data.data ) << endl;
		_currented_acked_message = data;
	}
   // Got some new data from the sensor
	else if ( _sensor_sock == socket )
	{
	   cout << "<base_station> Received new message with contents: " << ntohl( data.data ) << endl;

	   _current_message = data;
	   _num_messages_total++;

	   SocketUtils::SendMessage( _noisy_link_IP, _noisy_link_port, data, _noisy_link_sock );

	   _num_transmissions++;

		cout << "<base_station> Forwarded message " << ntohl( data.data ) << " to "
		  	  << _noisy_link_IP << ":" << _noisy_link_port << endl;

		// Set timeout to retransmit if we dont hear anything back from the noisy link
		SetTimeOut( timeout );
	}
}

// Called when socket utils detect console input
void ConsoleReceived( const char* message, bool* should_exit )
{
	if( strncmp( message, "q", 1 ) == 0 || strncmp( message, "Q", 1 ) == 0 )
	{
		*should_exit = true;
		cout << "<base_station> Base station transmission ratio: " << _num_transmissions / _num_messages_total << endl;
	}
}

// Called when socket utils detect time out
void RetransmitMessage( timeval** timeout )
{
   if( _current_message.data > _currented_acked_message.data )
   {
      cout << "<base_station> Retransmitting " << ntohl( _current_message.data ) << endl;
      SocketUtils::SendMessage( _noisy_link_IP, _noisy_link_port, _current_message, _noisy_link_sock );

      // Set timeout to retransmit if we still dont hear back from remote host
      SetTimeOut( timeout );

      _num_transmissions++;
   }
}

void SetTimeOut( struct timeval** timeout )
{
   timeval* val = new timeval();
   val->tv_sec = 0;
   val->tv_usec = RETRANSMIT_USEC;
   *timeout = val;
}
