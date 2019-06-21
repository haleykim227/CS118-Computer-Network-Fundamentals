#include "header.h"

#include <iostream>
#include <vector>
#include <cstdint>
#include <cstdio>

UDPHeader::UDPHeader() {
}

UDPHeader::UDPHeader(uint32_t seq_num, uint32_t ack, uint16_t conn_id, uint16_t ASF_flag){
    this->seq_num = seq_num;
    this->ack = ack;
    this->conn_id = conn_id;
    this->ASF_flag = ASF_flag;
}

//UDP packet constructor
UDPPacket::UDPPacket() {} 

UDPPacket::UDPPacket(std::vector<uint8_t>& seg) {
    //input: a fully built packet in build()
    //vector with at least 12 items representing 12 bytes
    uint32_t seq_temp;
    seq_temp = (seg[0]<<24)|(seg[1]<<16)|(seg[2]<<8)|seg[3];
    set_seq(seq_temp);
    uint32_t ack_temp;
    ack_temp = (seg[4]<<24)|(seg[5]<<16)|(seg[6]<<8)|seg[7];
    set_acknum(ack_temp);
    uint16_t id_temp;
    id_temp = (seg[8]<<8)|seg[9];
    set_connid(id_temp);
    uint16_t flag_temp;
    flag_temp = (seg[10]<<8)|seg[11];
    set_ASF_flag(flag_temp);
    
    int size = seg.size();
    if (size > 12) {
        this->send_data.clear();
        this->send_data.insert(this->send_data.begin(), seg.begin()+12, seg.end());
    }
}

UDPPacket::UDPPacket(UDPHeader h){
    this->UDP_head = h;
    this->send_data.clear();
}

UDPPacket::UDPPacket(UDPHeader encap, uint8_t* data, int size) {
    this->UDP_head = encap;
    this->send_data = std::vector<uint8_t>(data, data+size);
}

//destructor
UDPPacket::~UDPPacket() {}

//set fields
void UDPPacket::set_header(UDPHeader h) {
  this->UDP_head = h;
  return;
}

void UDPPacket::set_seq(uint32_t seq) {
  this->UDP_head.seq_num = seq;
  return;
}

void UDPPacket::set_acknum(uint32_t ack) {
  this->UDP_head.ack = ack;
  return;
}

void UDPPacket::set_connid(uint16_t connid) {
  this->UDP_head.conn_id = connid;
  return;
}

void UDPPacket::set_ASF_flag(uint16_t flags) {
    this->UDP_head.ASF_flag = flags;
    return;
}

void UDPPacket::set_data(std::vector<uint8_t>& data) {
  this->send_data = data;
}

//return important stuffs

UDPHeader UDPPacket::get_header() {
  return this->UDP_head;
}

uint32_t UDPPacket::return_seq() {
    return this->UDP_head.seq_num;
}
uint32_t UDPPacket::return_ack() {
    return this->UDP_head.ack;
}
uint16_t UDPPacket::return_id() {
    return this->UDP_head.conn_id;
}

uint16_t UDPPacket::get_ASF_flag() {
  return this->UDP_head.ASF_flag;
}

std::vector<uint8_t>* UDPPacket::get_data() {
  return &(this->send_data);
}

std::vector<uint8_t>* UDPPacket::get_bytes() {
  return &(this->eight_seg);
}

void UDPPacket::make(){
  this->eight_seg.clear();
  int i;
  for( i = 3; i >= 0; i--){
  	this->eight_seg.push_back((uint8_t) (this->UDP_head.seq_num >> (8*i)));
  }
  for( i = 3; i >= 0; i--){
  	this->eight_seg.push_back((uint8_t) (this->UDP_head.ack >> (8*i)));
  }
  for (i = 1; i >= 0; i--){
  	this->eight_seg.push_back((uint8_t) (this->UDP_head.conn_id >> (8*i)));
  }
  for (i = 1; i >= 0; i--){
  	this->eight_seg.push_back((uint8_t) (this->UDP_head.ASF_flag >> (8*i)));
  }
  this->eight_seg.insert(this->eight_seg.end(), this->send_data.begin(), this->send_data.end());
}


uint8_t* UDPPacket::get_beg(){
  return (&eight_seg[0]);
}

int UDPPacket::size()
{
  return eight_seg.size();
}
