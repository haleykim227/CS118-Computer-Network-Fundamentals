#ifndef SERVHELPER_INCLUDED
#define SERVHELPER_INCLUDED

#include "header.h"
#include <cstdint>
#include <vector>
#include <fstream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

class Server{
public:
  Server(int m_skfd, string dir, int size, sockaddr_storage addr);
  ~Server();
  bool get_fin();
  void recv(int m_skfd, uint32_t recv_seq, uint32_t recv_ack, uint16_t recv_id, uint16_t recv_flag, vector<uint8_t>* recv_data, int recv_size, sockaddr_storage their_addr);


private:
    int m_skfd;
    uint32_t seq_num;
    uint32_t ack;
    uint16_t conn_id;
    bool is_fin;
    int cwnd;
    int ssthresh;
    time_t timer;
    int pfd;
    uint32_t expect_seq;
};
#endif
