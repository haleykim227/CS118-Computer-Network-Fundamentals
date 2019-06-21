#include "servHelper.h"
#include "header.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <string>
#include <thread>
#include <unistd.h>
#include <fstream>
#include <cstring>
#include <ctime>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <new>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <sys/time.h>

using namespace std;

#define MAXSEQNUM 25600

class Server;
struct UDPHeader;
class UDPPacket;


Server::Server(int m_skfd, string dir, int size, sockaddr_storage addr)
{
  //fprintf(stdout, "hello\n");
  //initialize header values
  int init_id = size;
  int init_cwnd = 0;
  int init_ssthresh = 0;
  int init_seq_num = 12345;
  int s_seq_num = 4321;
   
  this->conn_id = init_id;
  this->cwnd = init_cwnd;
  this->ssthresh = init_ssthresh;
  this->expect_seq = init_seq_num + 1;
  this->is_fin = false;

  UDPHeader first_h(s_seq_num, this->expect_seq, init_id, 0x0006);
  UDPPacket first_pac(first_h);
  first_pac.make();
  int send_size;
  if((send_size = sendto(m_skfd, first_pac.get_beg(), first_pac.size(), 0,
              (struct sockaddr*)&(addr), (socklen_t)sizeof(addr))) < 0){
    fprintf(stdout,"ERROR: can't sendto for server\n");
    exit(1);
  }

  fprintf(stdout, "SEND %d %d %d %d", 
	    first_pac.return_seq(),
	    first_pac.return_ack(),
	    0,
	    0);
  uint16_t temp_flag = first_pac.get_ASF_flag();
  if ( temp_flag & 0x0004 ) {
     fprintf(stdout, "%s", " ACK");
  }
  if ( temp_flag & 0x0002) {
     fprintf(stdout, "%s", " SYN");
  }
  if ( temp_flag & 0x0001) {
     fprintf(stdout, "%s", " FIN");
  }
  fprintf(stdout, "\n");
  //printf("end of handshake server side.\n");

  string temp_file = dir + '/' + to_string(init_id) + ".file";
  char f_name[512];
  strcpy(f_name, temp_file.c_str());
  if((this->pfd = open(f_name, O_RDWR | O_CREAT | O_APPEND,
     S_IRWXU | S_IRWXG | S_IRWXO)) == -1){
    fprintf(stderr, "ERROR: Cannot open /etc/ptmp. Try again later.\n");
    exit(1);
  }

}


Server::~Server(){
}

bool Server::get_fin()
{
  return (this->is_fin);
}


void Server::recv(int m_skfd, uint32_t recv_seq, uint32_t recv_ack, uint16_t recv_id, uint16_t recv_flag, vector<uint8_t>* recv_data, int recv_size, sockaddr_storage addr)
{
   //out of order
   if (recv_seq != (this->expect_seq)) {    	
	if ( recv_flag & 0x0004 ) {
        	fprintf(stdout, "%s", " ACK");
    	}
    	if ( recv_flag & 0x0002) {
        	fprintf(stdout, "%s", " SYN");
    	}
    	if ( recv_flag & 0x0001) {
        	fprintf(stdout, "%s", " FIN");
    	}
    	fprintf(stdout, "\n"); 

   	//ack the expected Packet
    	UDPHeader r_head(4322, this->expect_seq, this->conn_id, 0x0004);
    	UDPPacket resend (r_head);
    	resend.make();
    	int ret;
    	ret = sendto(m_skfd, resend.get_beg(), resend.size(), 0,
                 (struct sockaddr*)&addr,
                 (socklen_t) sizeof(addr));
    	if (ret < 0) {
      		fprintf(stderr, "ERROR: resend error\n");
		exit(1);
    	}

    	fprintf(stdout, "SEND %d %d %d %d",
            resend.return_seq(),
            resend.return_ack(),
            0, 0);
        fprintf(stdout, "%s\n", " ACK");	
	return;
  }//end of out of order case.
  fprintf(stdout, "RECV %d %d %d %d", recv_seq, recv_ack, 
          this->cwnd, this->ssthresh);
   if ( recv_flag & 0x0004 ) {
	fprintf(stdout, "%s", " ACK");
   }
   if ( recv_flag & 0x0002) {
        fprintf(stdout, "%s", " SYN");
   }
   if ( recv_flag & 0x0001) {
        fprintf(stdout, "%s", " FIN");
   }
   fprintf(stdout, "\n");

  if (recv_flag & 0x0004){
      if( this->is_fin == true){
	   return;
      }
  }
  //else we know we have not erceived ack yet
  //so we send FIN again and go back to while loop.
  if (this->is_fin == true || (recv_flag & 0x0001)) {
      //printf("ACK not yet received.\n");
      UDPHeader header2(4322, recv_seq+1, recv_id, 0x0005);
      UDPPacket repac(header2);
      repac.make();
      uint32_t one = 4322;
      uint32_t two = recv_seq + 1;
      uint16_t three = recv_id;
      UDPHeader r_head( one, two, three, 0x0005);
      UDPPacket resend (r_head);
      resend.make();
      int ret;
      ret = sendto(m_skfd, resend.get_beg(), resend.size(), 0,
                 (struct sockaddr*)&addr,
                 (socklen_t) sizeof(addr));
      if (ret < 0) {
                fprintf(stderr, "ERROR: resend error\n");
                exit(1);
      }
      fprintf(stdout, "SEND %d %d %d %d",
            resend.return_seq(),
            resend.return_ack(),
            0, 0);
      fprintf(stdout, "%s\n", " ACK FIN");
      if( recv_flag & 0x0001){
	  this->expect_seq = recv_seq + 1;
    	  this->is_fin = true;
      }	
      return;
  }

  uint32_t one_seq = 4322;
  uint32_t two_ack;
  if( recv_seq + recv_size - 12 > MAXSEQNUM){
	two_ack = (recv_seq + recv_size - 12)%(MAXSEQNUM + 1);
  }
  else{
	two_ack = recv_seq +recv_size - 12;
  }
  //uint16_t three_id = recv_packet.return_id();

  UDPHeader r_head( one_seq, two_ack, recv_id, 0x0004);
  UDPPacket send (r_head);
  send.make();

  this->expect_seq = two_ack;
  //printf("expected syn = %d\n", expect_syn);

  int ret;
  //socklen_t addr_len = sizeof(addr);
  ret = sendto(m_skfd, send.get_beg(), send.size(), 0,
	       (struct sockaddr*)&addr, (socklen_t) sizeof(addr));
  if (ret < 0) {
      fprintf(stderr, "ERROR: resend error\n");
      exit(1);
  }  

  fprintf(stdout, "SEND %d %d %d %d",
            send.return_seq(),
            send.return_ack(),
            0, 0);
   uint16_t temp_flag = send.get_ASF_flag();
   if ( temp_flag & 0x0004 ) {
        fprintf(stdout, "%s", " ACK");
   }
   if ( temp_flag & 0x0002) {
        fprintf(stdout, "%s", " SYN");
   }
   if ( temp_flag & 0x0001) {
        fprintf(stdout, "%s", " FIN");
   }
   fprintf(stdout, "\n");

  // printf("successfully open the file.\n");
  vector<uint8_t> r_pac_data = *(recv_data);
  //printf("successful vector extraction.\n");
  uint8_t* temp = &r_pac_data[0];
  //     printf("datavec conversion.\n");
  int w_ret = write(pfd, temp, r_pac_data.size());
  if (w_ret < 0) {
   	fprintf(stderr,"ERROR: unable to write to file\n");
	exit(1);
  }
}
