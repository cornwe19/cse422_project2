#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>

#define ONE_SEC 1000000.0

struct unicast_pkt {
  unsigned int data;
};

int sock_fd;
static unsigned int current_key;
struct sockaddr_in dest_addr;
int msg_sent, total_msg;

static void alarm_handler(int signo) {
  char buffer[1024];
  struct unicast_pkt udp_pkt;

  if(msg_sent == total_msg) {
    ualarm(0,0);
    exit(1);
  }

  udp_pkt.data = htonl(current_key);

  sendto(sock_fd, &udp_pkt, sizeof(udp_pkt), 0, (struct sockaddr *) &dest_addr, sizeof(dest_addr));
  printf("UDP packet sent to %s on port %d with data = %u\n",
          inet_ntop(AF_INET, &dest_addr.sin_addr, buffer, sizeof(buffer)),
          ntohs(dest_addr.sin_port), current_key);

  current_key++;
  msg_sent++;
}

int main (int argc, const char * argv[]) {

  unsigned short UDP_port;
  const char* dest_IP;
  const int sock_reuse_opt = 1;
  int alarm_duration;
  

  if(argc < 5 || argc > 6 || atof(argv[3])>1.0) {
    printf("USAGE: sensor_network DEST_IP DEST_PORT MSG_INTERVAL_IN_SEC(<=1.0) TOTAL_MSG <START_SEQ>\n");
    return 1;
  }

  if(argc == 6) {
    current_key = atoi(argv[5]);
  } else {
    current_key = random();
  }

  dest_IP = argv[1];
  UDP_port = (short) atoi(argv[2]);
  alarm_duration = atof(argv[3])*ONE_SEC;
  total_msg = atoi(argv[4]);
  msg_sent = 0;
  
  printf("Sensor network running\n");
  
  // clear and set destination address (base station address)
  bzero(&dest_addr, sizeof(dest_addr));
  dest_addr.sin_family = AF_INET;
  dest_addr.sin_port = htons(UDP_port);
  inet_pton(AF_INET, dest_IP, &dest_addr.sin_addr);
  // create a socket  
  sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
  setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse_opt, sizeof(sock_reuse_opt));

  // create alarm
  signal(SIGALRM, alarm_handler);
  ualarm(1,alarm_duration);
  int i = 0;
  while(i < total_msg) {
    pause();
    i++;
  }
  return 0;
}
