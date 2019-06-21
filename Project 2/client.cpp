#include "header.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include <stdlib.h>
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
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fstream>
#include <signal.h>
#include <sstream>
#include <fcntl.h>
#include <new>	

#define MAXBUFSIZE 524
#define MAXSEQNUM 25600
#define MAXCWNDSIZE 10240

using namespace std;

struct UDPHeader;
class UDPPacket;

void error_msg(const char *msg){
	fprintf(stderr,"%s", "ERROR: ");
	fprintf(stderr, "%s", msg);
	exit(1);
}

int main(int argc, char** argv){ 
   //local vairables
   int m_skfd;

   uint32_t cwnd = 512;
   uint32_t ssthresh = 5120;
   uint32_t seq_num = 12345;
   uint32_t ack = 0;
   uint32_t conn_id = 0;
   uint8_t buffer[524];
   ifstream send_file;

   struct addrinfo hints;
   struct addrinfo* infoptr;
   
   if (argc != 4){
	fprintf(stderr, "ERROR: there should be 3 arguments!\n");
	exit(1);
   }
   
   //int portno = atoi(argv[2]);
   //start connection
   memset(&hints, 0, sizeof hints);
   hints.ai_family = AF_INET;
   hints.ai_socktype = SOCK_DGRAM;

   int a_info = getaddrinfo(argv[1], argv[2], &hints, &infoptr);
   if(a_info != 0){
	error_msg("unable to get addr info\n");	
   }
   
   m_skfd = socket(infoptr->ai_family, infoptr->ai_socktype, infoptr->ai_protocol);
   if(m_skfd < 0){
	error_msg("unable to create socket\n");
   }
   
   // allow others to reuse the address
   int yes = 1;
   if (setsockopt(m_skfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    	error_msg("unable to setsockopt\n");
   }
   
   //set retransmission timeout to 0.5 sec
   struct timeval timeout;
   timeout.tv_sec= 0;
   timeout.tv_usec = 500000;
   if (setsockopt(m_skfd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ){
	error_msg("unable to set retrans timeout\n");
   }
   
   //bind address to socket
   struct sockaddr_in c_addr;
   socklen_t addr_size = sizeof(c_addr);
   memset((char*)&c_addr,0, sizeof(c_addr));
   c_addr.sin_family = AF_INET;
   c_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   c_addr.sin_port = htons(0);
   if( bind(m_skfd, (struct sockaddr*)&c_addr, sizeof(c_addr)) < 0){
	error_msg("unable to bind\n");
   }
   //open file
   send_file.open(argv[3], ios::in|ios::binary);
   if(!send_file.is_open()){
	error_msg("Can't open or edit file\n");
   }
   
   //3 way handshake
   uint16_t syn = 0x0002;
   UDPHeader m_header(seq_num, ack, conn_id, syn);
   UDPPacket m_packet(m_header);
   m_packet.make(); 
    
   if(sendto(m_skfd, m_packet.get_beg(), m_packet.size(), 0, infoptr->ai_addr, infoptr->ai_addrlen) == -1){
        error_msg("sendto somehow failed in 3 way handshake\n");
   }
   fprintf(stdout, "SEND %d %d %d %d %s\n", seq_num, ack, cwnd, ssthresh, "SYN");
    seq_num++;
    
    int recv_size;
    recv_size = recvfrom(m_skfd, buffer, MAXBUFSIZE, 0, (struct sockaddr*)&c_addr, &addr_size);
    if (recv_size < 0) {
        error_msg("recv somehow failed in 3 way handshake\n");
    }
    
    //receive packet from server
    vector<uint8_t> temp(buffer, buffer+recv_size);
    UDPPacket recv_packet(temp);
    //output msg
    fprintf(stdout, "RECV %d %d %d %d", 
	    recv_packet.return_seq(),
	    recv_packet.return_ack(),
            cwnd, ssthresh);
    uint16_t temp_flag = recv_packet.get_ASF_flag();
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

    //check if the connection is just ACK
    if ( temp_flag & 0x0006 ){
		if(recv_packet.return_ack() == seq_num){
			conn_id = recv_packet.return_id();
        		ack = recv_packet.return_seq()+1;
        		//cwnd += 512;
		}
		else{
			error_msg("wrong return for handshake\n");
		}
    }
    else {
        error_msg("wrong return for handshake\n");
    }
    //handshake finished!

    //send begin!/////////////////////////
    //////////////////////////////////////

    send_file.seekg(0,ios::end);
    uint32_t file_size = send_file.tellg();
    uint32_t send_size = 0;

    send_file.seekg(0, ios::beg); //set file read to start from beg
    memset(buffer, '\0', 524); //clear buffer
    //send_file.read((char *)buffer, 512); 
    //build packet
    UDPHeader first_h;
    first_h = UDPHeader(seq_num, ack, conn_id, 0x0004);
    UDPPacket first_pac;
    first_pac = UDPPacket( first_h);
    first_pac.make();
    if(sendto(m_skfd, first_pac.get_beg(), first_pac.size(), 0, infoptr->ai_addr, infoptr->ai_addrlen) == -1){
        error_msg("first send somehow failed...\n");
   } 
    fprintf(stdout, "SEND %d %d %d %d %s\n",
            seq_num, ack,cwnd, ssthresh, "ACK");
    
    //to keep track of the bytes
    uint32_t s_reset = 0;
    uint32_t a_reset = 0;
    uint32_t first_seq = seq_num;
    uint32_t recv_ack = seq_num;

    uint32_t s_lim_add = (MAXSEQNUM + 1)*(s_reset-a_reset);
    uint32_t send_lim = seq_num - recv_ack + s_lim_add;
    
    int time_count = 0;
    UDPPacket temp_pac;
    int s_size; 
    while (file_size - send_size > 0) { //more data to send 
            if ((send_lim < file_size - send_size) && (send_lim < cwnd)) {
                //send a packet
                memset(buffer, '\0', sizeof(uint8_t)*524);
                send_file.read((char *)buffer, 512);
		UDPHeader temp_h;
            	temp_h = UDPHeader(seq_num, 0, conn_id, 0x0000);
            	uint32_t temp_size = file_size - send_size - send_lim;
            	if( temp_size > 512){
                	temp_size = 512;
            	}
            	temp_pac = UDPPacket( temp_h, (uint8_t *)buffer, temp_size);
            	temp_pac.make();
                s_size = sendto(m_skfd, temp_pac.get_beg(), temp_pac.size(), 0,
                             infoptr->ai_addr, infoptr->ai_addrlen);
                if (s_size == -1) {
                    error_msg("cannot sendto file\n");
                }
                fprintf(stdout, "SEND %d %d %d %d %s\n",
                        seq_num, 0, cwnd, ssthresh, "");
                //seq_num += (s_size-12);
                if (seq_num + s_size - 12 > MAXSEQNUM) {
                    seq_num = (seq_num + s_size - 12) % (MAXSEQNUM + 1);
                    s_reset++;
                }
		else{
		    seq_num = seq_num + s_size - 12;
		}
		s_lim_add = (MAXSEQNUM + 1)*(s_reset-a_reset);
    		send_lim = seq_num - recv_ack + s_lim_add;
            }
            else {
                s_size = recvfrom(m_skfd, buffer, 524 , 0, (struct sockaddr*)&(c_addr), &addr_size);
                if (s_size == -1) { //start timeout!
                    time_count += 1;
                    if (time_count > 19) { // 0.5*20 = 10 sec
			close(m_skfd);
                        error_msg("10 sec timeout expired. Exiting.\n");
                    }
                   
                    ssthresh = cwnd/2; cwnd = 512; send_lim = 0;
                    //start to resend packet
                    if (seq_num < recv_ack)
                        s_reset = s_reset - 1;
                    seq_num = recv_ack;
                    memset(buffer, '\0', sizeof(uint8_t)*524);
                    send_file.seekg (send_size, send_file.beg);
                    send_file.read((char *)buffer, 512);
            	    UDPHeader temp_h;
		    temp_h = UDPHeader(seq_num, 0, conn_id, 0x0000);
            	    uint32_t temp_size = file_size - send_size - send_lim;
            	    if( temp_size > 512){
                	temp_size = 512;
            	    }
            	    temp_pac = UDPPacket( temp_h, (uint8_t *)buffer, temp_size);
            	    temp_pac.make();

                    s_size = sendto(m_skfd, temp_pac.get_beg(), temp_pac.size(), 0,
                                 infoptr->ai_addr, infoptr->ai_addrlen);
                    if (s_size == -1) {
                        error_msg(" resend failed\n");
                    }
                    fprintf(stdout, "SEND %d %d %d %d %s\n",
                            seq_num, 0, cwnd, ssthresh, "DUP");
                    if (seq_num + s_size - 12 > MAXSEQNUM) {
                        seq_num = (seq_num + s_size - 12) % (MAXSEQNUM + 1);
                        s_reset++;
                    }
		    else{
			seq_num = seq_num + s_size - 12;
		    }
		    s_lim_add = (MAXSEQNUM + 1)*(s_reset-a_reset);
                    send_lim = seq_num - recv_ack + s_lim_add;
                    continue;
                }
                
                std::vector<uint8_t> vec(buffer, buffer+s_size);
                UDPPacket r_pac(vec);
                
                fprintf(stdout, "RECV %d %d %d %d",
                        r_pac.return_seq(),
                        r_pac.return_ack(),
                        cwnd, ssthresh);
		uint16_t temp_flag = r_pac.get_ASF_flag();
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
              
                // ack exceeds MAX_ACK
                if ((temp_flag & 0x0004) && r_pac.return_ack() < recv_ack && (recv_ack - r_pac.return_ack()) >= (MAXSEQNUM / 2)) {
                    if (cwnd < ssthresh) //slow start
                        cwnd += 512;
                    else  // congest control
                        cwnd += (512*512)/cwnd;
                    if (cwnd > MAXCWNDSIZE)
                        cwnd = MAXCWNDSIZE;
                    a_reset += 1;
                    recv_ack = r_pac.return_ack();
		    send_size = (recv_ack+(a_reset*(MAXSEQNUM + 1))) - first_seq;
		    s_lim_add = (MAXSEQNUM + 1)*(s_reset-a_reset);
                     send_lim = seq_num - recv_ack + s_lim_add;
                }
                // no abnormal
                else if((temp_flag & 0x0004) && r_pac.return_ack() > recv_ack) {
		    if (cwnd < ssthresh) //slow start
                    	cwnd += 512;
		    else  //congest control
			cwnd += (512*512)/cwnd;
                    if (cwnd > MAXCWNDSIZE)
                        cwnd = MAXCWNDSIZE;
                    recv_ack = r_pac.return_ack();
		    send_size = (recv_ack+(a_reset*(MAXSEQNUM + 1))) - first_seq;
                    s_lim_add = (MAXSEQNUM + 1)*(s_reset-a_reset);
                    send_lim = seq_num - recv_ack + s_lim_add;
                }
		/*int16_t temp_ack = recv_ack - r_pac.return_ack();
                //duplicate ack
                if ((temp_flag & 0x0004) && temp_ack == 0) {}
                else if((temp_flag & 0x0004) && temp_ack>0 && temp_ack<51200 ) {}
                else if ((temp_flag & 0x0004) && temp_ack>0 && temp_ack>= 51200) 		    {
                    if (cwnd < ssthresh) //slow start
                        cwnd += 512;
                    else  // congest control
                        cwnd += (512*512)/cwnd;
                    if (cwnd > 51200)
                        cwnd = 51200;
                    a_reset++;
                    recv_ack = r_pac.return_ack();
		    send_size = (recv_ack+(a_reset*102401)) - first_seq;
                    s_lim_add = 102401*(s_reset-a_reset);
                    send_lim = seq_num - recv_ack + s_lim_add;
                }
                //normal ack
                else if((temp_flag & 0x0004) && temp_ack > 0) {
		    if (cwnd < ssthresh) //slow start
                    	cwnd += 512;
		    else  //congest control
			cwnd += (512*512)/cwnd;
                    if (cwnd > 51200)
                        cwnd = 51200;
                    recv_ack = r_pac.return_ack();
		    send_size = (recv_ack+(a_reset*102401)) - first_seq;
                    s_lim_add = 102401*(s_reset-a_reset);
                    send_lim = seq_num - recv_ack + s_lim_add;
                }*/
		
                time_count = 0;
            }
        }       
               
    //Close connection!//////////////////
    /////////////////////////////////////
    /////////////////////////////////////
    int temp_seq = seq_num;    
    //send FIN
    UDPHeader fin_head(seq_num, 0, conn_id, 0x0001);
    UDPPacket fin_pac(fin_head);
    fin_pac.make();
    
    s_size = sendto(m_skfd, fin_pac.get_beg(), fin_pac.size(), 0, infoptr->ai_addr, infoptr->ai_addrlen);
    if (s_size == -1){
        error_msg( " cannot sendto the last fin pac\n");
    }
    fprintf(stdout, "SEND %d %d %d %d %s\n",
            seq_num, 0, cwnd, ssthresh, "FIN");

    seq_num += 1;

    if (seq_num > MAXSEQNUM)
      seq_num = seq_num % (MAXSEQNUM + 1); 
    
    //recv ACKFIN
    time_count = 0;
    
    struct timeval t1, t2;
    double elapsedTime;
    gettimeofday(&t1, NULL);
    bool s_ack = false;
    bool r_ack = false;
    
    do {
        recv_size = recvfrom(m_skfd, buffer, MAXBUFSIZE, 0, (struct sockaddr*)&c_addr, &addr_size);
        if (recv_size == -1) {
            //timeout
            time_count += 1;
            if (time_count > 19) {
                 close(m_skfd);
                 error_msg("recv timeout... Exit.\n");
            }
            //set cwnd and ssthresh
            ssthresh = cwnd/2;
            cwnd = 512;
            //resend FIN packet
            if (s_ack == false) {
                seq_num = temp_seq;
                s_size = sendto(m_skfd, fin_pac.get_beg(), fin_pac.size(), 0, infoptr->ai_addr, infoptr->ai_addrlen);
    		if (s_size == -1){
        	    error_msg( " cannot sendto the last fin pac\n");
    		}
    		fprintf(stdout, "SEND %d %d %d %d %s\n", seq_num, 0, cwnd, ssthresh, "FIN DUP");
                if (seq_num + 1> MAXSEQNUM)
		  seq_num = (seq_num+1) % (MAXSEQNUM + 1);
		else
		    seq_num = seq_num + 1;
            }
            if (r_ack == false) {
                elapsedTime = 0;
                continue;
            }
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;
            continue;
        }
        
        //receive packet from server
        vector<uint8_t> fin_buf(buffer, buffer+recv_size);
        UDPPacket fin_pac2(fin_buf);
 	uint16_t pac2_flag = fin_pac2.get_ASF_flag();
        //FINACK
        if ( fin_pac2.return_id() == conn_id && (pac2_flag && 0x0005))
        {
            //debug msg
            ack = fin_pac2.return_seq()+1;
            fprintf(stdout, "RECV %d %d %d %d",
                    fin_pac2.return_seq(), fin_pac2.return_ack(),
                    cwnd, ssthresh);
	    if ( pac2_flag & 0x0004 ) {
                fprintf(stdout, "%s", " ACK");
            }
            if ( pac2_flag & 0x0002) {
                fprintf(stdout, "%s", " SYN");
            }
            if ( pac2_flag & 0x0001) {
                fprintf(stdout, "%s", " FIN");
            }
            fprintf(stdout, "\n");

            time_count = 0;

            /*if (r_ack == false) {
                if (cwnd < ssthresh)
                    cwnd += 512;
                else
                    cwnd += (512*512)/cwnd;
            }*/
            r_ack = true;
        }
        //ACK
        else if ( fin_pac2.return_id() == conn_id && (pac2_flag & 0x0004))
        {
            //debug msg
            ack = fin_pac2.return_seq()+1;
            fprintf(stdout, "RECV %d %d %d %d",
                    fin_pac2.return_seq(), fin_pac2.return_ack(),
                    cwnd, ssthresh );
	    if ( pac2_flag & 0x0004 ) {
                fprintf(stdout, "%s", " ACK");
            }
            if ( pac2_flag & 0x0002) {
                fprintf(stdout, "%s", " SYN");
            }
            if ( pac2_flag & 0x0001) {
                fprintf(stdout, "%s", " FIN");
            }
            fprintf(stdout, "\n");

            if (r_ack == false) {
                if (cwnd >= ssthresh)
                    cwnd += (512*512)/cwnd;
                else
                    cwnd += 512;
            }
            time_count = 0;
            r_ack = true;
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
            continue;
        }
        //FIN
        else if (fin_pac2.return_id() == conn_id && (pac2_flag & 0x0001))
        {
            //debug msg
            ack = fin_pac2.return_seq()+1;
            fprintf(stdout, "RECV %d %d %d %d",
                    fin_pac2.return_seq(), fin_pac2.return_ack(),
                    cwnd, ssthresh );
            if ( pac2_flag & 0x0004 ) {
                fprintf(stdout, "%s", " ACK");
            }
            if ( pac2_flag & 0x0002) {
                fprintf(stdout, "%s", " SYN");
            }
            if ( pac2_flag & 0x0001) {
                fprintf(stdout, "%s", " FIN");
            }
            fprintf(stdout, "\n");
            time_count = 0;
        }
        else {
            fprintf(stdout, "RECV %d %d %d %d",
                    fin_pac2.return_seq(), fin_pac2.return_ack(),
                    cwnd, ssthresh );
            if ( pac2_flag & 0x0004 ) {
                fprintf(stdout, "%s", " ACK");
            }
            if ( pac2_flag & 0x0002) {
                fprintf(stdout, "%s", " SYN");
            }
            if ( pac2_flag & 0x0001) {
                fprintf(stdout, "%s", " FIN");
            }
            fprintf(stdout, "\n");
            time_count = 0;
            gettimeofday(&t2, NULL);
            elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 +(t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
            continue;
        }
        
        
        //send ACK
        fin_pac = UDPPacket( UDPHeader(seq_num, ack, conn_id, 0x0004));
        fin_pac.make(); 
        
        s_size = sendto(m_skfd, fin_pac.get_beg(), fin_pac.size(), 0,
                     infoptr->ai_addr, infoptr->ai_addrlen);
        if (s_size == -1){
            error_msg("unable to send ack for fin\n");
        }
        fprintf(stdout, "SEND %d %d %d %d %s\n",
                seq_num, ack, cwnd, ssthresh, "ACK");
        s_ack = true;
        gettimeofday(&t2, NULL);
        elapsedTime = (t2.tv_sec - t1.tv_sec) * 1000.0 + (t2.tv_usec - t1.tv_usec) / 1000.0;   // us to ms
     } while(elapsedTime <= 2000.0); //2 sec wait
}
