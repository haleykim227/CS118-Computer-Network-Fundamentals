#ifndef HEADER_INCLUDED
#define HEADER_INCLUDED
#include <cstdint>
#include <vector>

using namespace std;

struct UDPHeader {
    UDPHeader();
    UDPHeader(uint32_t seq_num, uint32_t ack, uint16_t conn_id, uint16_t flags);
    uint32_t seq_num;
    uint32_t ack;
    uint16_t conn_id;
    uint16_t ASF_flag;
};

class UDPPacket {
  public:
  //constructor and destructor
  //constructor with data specified
    UDPPacket();
    ~UDPPacket();
    UDPPacket(std::vector<uint8_t>& seg);
    UDPPacket(UDPHeader encap);
    UDPPacket(UDPHeader encap, uint8_t* data, int size);
    
  //set header info
    void set_header(UDPHeader head);
    void set_seq(uint32_t seq_num);
    void set_acknum(uint32_t ack);
    void set_connid(uint16_t conn_id);
    void set_data(std::vector<uint8_t>& data);
    void set_ASF_flag(uint16_t asf);
    
  //return header info
    UDPHeader get_header();
    uint32_t return_seq();
    uint32_t return_ack();
    uint16_t return_id();
    int size();
    uint16_t get_ASF_flag();
    vector<uint8_t>* get_data();
    vector<uint8_t>* get_bytes();
    uint8_t* get_beg();

    //build the packet into bytewise vector
    void make();

  private:
    UDPHeader UDP_head;
    vector<uint8_t> send_data;
    vector<uint8_t> eight_seg;
};
#endif 
