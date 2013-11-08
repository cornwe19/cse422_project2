#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <strings.h>
#include <limits.h>

struct unicast_pkt {
  unsigned int data;
};

int main (int argc, const char * argv[]) {
  unsigned short listen_port_bs, listen_port_rh, rh_port;
  double drop_frac;
  const char* rh_ip;
  int total_msg_forwarded_to_BS  = 0;
  int total_msg_received_from_BS = 0;
  int total_msg_forwarded_to_RH  = 0;
  int total_msg_received_from_RH = 0;
  if(argc != 6) {
    printf("USAGE: noisy_link LISTEN_PORT_FOR_BS LISTEN_PORT_FOR_RH RH_IP RH_PORT DROP_FRAC\n");
    return 1;
  }
  
  listen_port_bs = (short) atoi(argv[1]);
  listen_port_rh = (short) atoi(argv[2]);
  rh_ip = argv[3];
  rh_port = (short) atoi(argv[4]);
  drop_frac = atof(argv[5]);
  
  if(drop_frac > 1.0 || drop_frac < 0.0) {
    printf("DROP_FRAC must be between 0 and 1\n");
    return 2;
  }
  
  printf("noisy_link in listening to the base station on port %d\n and remote host on port %d.\n%3.2f%% of received packets are being forwarded.\n", 
         listen_port_bs, listen_port_rh, 100-drop_frac*100);
  
  // define address stucture
  struct sockaddr_in bs_listen_addr, rh_listen_addr, bs_addr, rh_addr;
  socklen_t bs_addr_length = sizeof(bs_addr);
  struct unicast_pkt udp_pkt;
  
  // clear and set base station address
  bzero(&bs_listen_addr, sizeof(bs_listen_addr));
  bs_listen_addr.sin_family = AF_INET;
  bs_listen_addr.sin_port = htons(listen_port_bs);
  bs_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  // clear and set remote host address
  bzero(&rh_listen_addr, sizeof(rh_listen_addr));
  rh_listen_addr.sin_family = AF_INET;
  rh_listen_addr.sin_port = htons(listen_port_rh);
  rh_listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  
  // clear and set receiver(remote host) addresses
  bzero(&rh_addr, sizeof(rh_addr));
  rh_addr.sin_family = AF_INET;
  rh_addr.sin_port = htons(rh_port);
  inet_pton(AF_INET, rh_ip, &rh_addr.sin_addr);
    
  /******** setup UDP listen socket for base station *********/
  int sock_fd_listen_bs = socket(AF_INET, SOCK_DGRAM, 0);
  int sock_reuse_opt = 1;
  setsockopt(sock_fd_listen_bs, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse_opt, sizeof(sock_reuse_opt));
  
  if(bind(sock_fd_listen_bs, (struct sockaddr*) &bs_listen_addr, sizeof(bs_listen_addr)) != 0) {
    printf("UDP Bind failed on port %d\n", listen_port_bs);
    return 3;
  }
  
  /******** setup UDP listen socket for remote host *********/
  int sock_fd_listen_rh = socket(AF_INET, SOCK_DGRAM, 0);
  sock_reuse_opt = 1;
  setsockopt(sock_fd_listen_rh, SOL_SOCKET, SO_REUSEADDR, (char *) &sock_reuse_opt, sizeof(sock_reuse_opt));
  
  if(bind(sock_fd_listen_rh, (struct sockaddr*) &rh_listen_addr, sizeof(rh_listen_addr)) != 0) {
    printf("UDP Bind failed on port %d\n", listen_port_rh);
    return 4;
  }
    
  int max_fd;
  if(sock_fd_listen_bs > sock_fd_listen_rh)
    max_fd = sock_fd_listen_bs+1;
  else 
    max_fd = sock_fd_listen_rh+1;
  
  fd_set readset;
  FD_ZERO(&readset);
  // run in an infinite loop until there is any error or user presses 'q' or 'Q' from the command prompt	
  while(1) {    
    FD_SET(sock_fd_listen_bs, &readset);
    FD_SET(sock_fd_listen_rh, &readset);
    FD_SET(0, &readset);
    
    // wait on receiving sockets [PLEASE SEE HOW select() works]
    select(max_fd, &readset, NULL, NULL, NULL);
    
    bzero(&udp_pkt, sizeof(udp_pkt));
    // if there is data from base station, send it to remote host	
    if(FD_ISSET(sock_fd_listen_bs, &readset)) {
      bzero(&bs_addr, sizeof(bs_addr));
      bzero(&udp_pkt, sizeof(udp_pkt));
      if(recvfrom(sock_fd_listen_bs, &udp_pkt, sizeof(udp_pkt), 0, (struct sockaddr*) &bs_addr, &bs_addr_length) < 0) {
        printf("UDP base station receive failed\n");
        return 7;
      }
      total_msg_received_from_BS++;
      
      //  drop message? if greater than drop_frac, do not drop
      if((double)random()/(double)INT_MAX > drop_frac) {
        if(sendto(sock_fd_listen_rh, &udp_pkt, sizeof(udp_pkt), 0, (struct sockaddr*) &rh_addr, sizeof(rh_addr)) < 0) {
          printf("Forward to remote host failed\n");
          return 8;    
        }
//        printf("Message forwarded\n");
          total_msg_forwarded_to_RH++;
      } else {
//        printf("Message dropped\n");
      }
      FD_CLR(sock_fd_listen_bs, &readset);
    }
    // if there is data from remote host,
    if(FD_ISSET(sock_fd_listen_rh, &readset)) {
      struct sockaddr_in rh_sending_addr;
      unsigned int rh_sending_addr_lenght = sizeof(rh_sending_addr);
      bzero(&rh_sending_addr, sizeof(rh_sending_addr));
      bzero(&udp_pkt, sizeof(udp_pkt));
      if(recvfrom(sock_fd_listen_rh, &udp_pkt, sizeof(udp_pkt), 0, (struct sockaddr*) &rh_sending_addr, &rh_sending_addr_lenght) < 0) {
        printf("UDP remote host receive failed\n");
        return 9;
      }
      total_msg_received_from_RH++;
      
      //  drop message?if greater than drop_frac, do not drop
      if((double)random()/(double)INT_MAX > drop_frac) {
        if(sendto(sock_fd_listen_bs, &udp_pkt, sizeof(udp_pkt), 0, (struct sockaddr*) &bs_addr, sizeof(bs_addr)) < 0) {
          printf("Forward to base station failed\n");
          return 10;    
        }
//        printf("Message forwarded\n");
          total_msg_forwarded_to_BS++;
      } else {
//        printf("Message dropped\n");
      }
      FD_CLR(sock_fd_listen_rh, &readset);
    }
    // if there is data from console, terminate the program
    if(FD_ISSET(0, &readset)){
      char letter[1];
      scanf("%s", letter);
      if(letter[0] == 'q' || letter[0] == 'Q') {
        printf("%d total messages received from BS,\t%d total message forwarded to BS\n", total_msg_received_from_BS, total_msg_forwarded_to_BS);

        printf("%d total messages received from RH,\t%d total message forwarded to RH\n", total_msg_received_from_RH, total_msg_forwarded_to_RH);
        return 11;
      }
      FD_CLR(0, &readset);
    }
  }
  return 0;
}
