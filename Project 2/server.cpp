#include "header.h"
#include "servHelper.h"
#include <string>
#include <thread>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <fstream>
#include <csignal>
#include <thread>
#include <stdio.h>

using namespace std;
//to do: TCP specific variables
//signal handling
std::vector<Server> serv_connections;
int m_skfd;
int cwnd = 0;
int ssthresh = 0;


void sig_handler(int sig)
{
  if (sig == SIGINT )
    cerr << endl;
  else if(sig == SIGQUIT )
    cerr << endl;
  else if (sig == SIGTERM)
    cerr <<endl;
  exit(0);
}

void error_msg(const char *msg){
    fprintf(stderr, "ERROR: ");
    fprintf(stderr, "%s", msg);
    exit(1);
}


int main(int argc, char* argv[])
{
  if (signal(SIGINT, sig_handler) == SIG_ERR) {
    printf("ERROR: cannot catch SIGINT\n");
  }
  if (argc != 2) {
    printf("usage: %s <portno>\n", argv[0]);
    exit(1);
  }
  
  int port_num = atoi(argv[1]);
  if ( port_num <= 1023 || port_num >= 65535){
      error_msg(" portnumber must be between 1023 and 65535.\n");
  }

  struct addrinfo hints, *infoptr;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_DGRAM;
  hints.ai_flags = AI_PASSIVE;

  if( getaddrinfo(NULL, argv[1], &hints, &infoptr) != 0 ){
    error_msg(" can't getaddrinfo \n");
  }

  m_skfd = socket(infoptr->ai_family, infoptr->ai_socktype, infoptr->ai_protocol);
  if (m_skfd < 0) {
    error_msg("Can't create socket\n");
  }

  if( bind(m_skfd, infoptr->ai_addr, infoptr->ai_addrlen) < 0){
    fprintf(stderr, "ERROR: binding.\n");
  }


  //keep recvfrom and see if we have new connetions
  while(1) {
    struct sockaddr_storage their_addr;
    socklen_t addr_len = sizeof(their_addr);
    int recvbytes;
    uint8_t buffer[524];

    if ((recvbytes = recvfrom(m_skfd,buffer, 524,0,
			      (struct sockaddr*)&their_addr, &addr_len))==-1)
      {
	       perror("recvfrom");
	        exit(1);
      }

    vector<uint8_t> vec(buffer, buffer+recvbytes);
    UDPPacket recv_packet(vec);

    if (recv_packet.return_id() == 0) {
      //we have a new connection.
      int Vsize = serv_connections.size() + 1;
      cout <<"RECV 12345 0 0 0 SYN"<<endl;
      Server new_conn(m_skfd, ".", Vsize, their_addr);
      serv_connections.push_back(new_conn);
      continue;
    }
    if ((serv_connections[recv_packet.return_id()-1]).get_fin()){
	if ( recv_packet.get_ASF_flag() & 0x0001) {
      		fprintf(stderr,"ERROR: connection error\n");
      		exit(1);
    	}
    }
    //connection already established, call the function.
    uint32_t recv_seq = recv_packet.return_seq();
    uint32_t recv_ack = recv_packet.return_ack();
    uint16_t recv_id = recv_packet.return_id();
    uint16_t recv_flag = recv_packet.get_ASF_flag();
    vector<uint8_t>* recv_data = recv_packet.get_data();    
    (serv_connections[recv_packet.return_id()-1]).recv(m_skfd, recv_seq, recv_ack, recv_id, recv_flag, recv_data, recvbytes, their_addr);
  }//end of while loop


  return 0;
}
