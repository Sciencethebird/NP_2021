# pragma once
#include <string>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <sstream>
#include <map>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>

#define debug(a) std::cerr << #a << " = " << a << std::endl
#define CONNECT 1
#define BIND 2
#define REQ_GRANTED 90
#define REQ_DENIED 91

namespace ip = boost::asio::ip;

bool is_url(u_char c){
    return c >=32 && c <=127;
}
struct sock4a_request_parser{
public:
    //enum {VN = 0, CD = 1, DSTPORT = 2, DSTIP = 4, USERID = 8};
    /*
    	        +----+----+----+----+----+----+----+----+----+----+....+----+
		        | VN | CD | DSTPORT |      DSTIP        | USERID       |NULL|
		        +----+----+----+----+----+----+----+----+----+----+....+----+
    # of bytes:    1    1      2              4           variable       1
    VN: 1 byte, SOCKS protocol version number and should be 4
    CD: 1 byte, SOCKS command code, CONNECT = 1, BIND = 2
    DSTPORT: 2 byte
    DSTIP:   4 byte, 0.0.0.1 means the client could not resolve domain name
    */
public:
    sock4a_request_parser(u_char* request_raw){
        //print_raw_packet(request_raw);
        VN = (uint)request_raw[0];
        CD = (uint)request_raw[1];
        DST_PORT = (request_raw[2] << 8) | (request_raw[3]);
        DST_PORT_RAW[0] = request_raw[2];
        DST_PORT_RAW[1] = request_raw[3];
        for(int i= 0; i<4; i++){
            DST_IP[i] = (uint) request_raw[4+i];
            DST_IP_RAW[i] = request_raw[4+i];
        }

        request_url = "";
        //std::cout << "asdf" << std::endl;
        int start = 8;
        for(int i = start; i < max_length; i++){
            if((int)request_raw[i] == 0) {
                start = i+1;
                break;
            }
        }
       // std::cout << "start: " << start << std::endl;

        for(int i = start; i < max_length; i++){
            //std::cout << request_raw[i];// << std::endl;
            if(is_url(request_raw[i])){
                request_url+=(u_char)request_raw[i];
            }
            if((int)request_raw[i] == 0) break;
        }
        //print_request();
    }
    void print_request(std::string source_ip, int source_port, int reply){
        // << "<version>: " << VN << std::endl;
        printf("<S_IP>: %s\n", source_ip.c_str());
        printf("<S_PORT>: %d\n", source_port);
        printf("<D_IP>: %u.%u.%u.%u\n", DST_IP[0], DST_IP[1], DST_IP[2], DST_IP[3]);
        printf("<D_port>: %d\n", DST_PORT);
        if(CD == CONNECT)
            printf("<Command>: %s\n", "CONNECT");
        else if(CD == BIND)
            printf("<Command>: %s\n", "BIND");
        if(reply == REQ_GRANTED)
            printf("<Reply>: %s\n", "Accept");
        else if(reply == REQ_DENIED)
            printf("<Reply>: %s\n", "Reject");
        std::cout << std::endl;
    }
    std::string get_ip_str(){
        if(request_url.size() != 0) return request_url;
        else return std::to_string(DST_IP[0])+"."+std::to_string(DST_IP[1])+"."+std::to_string(DST_IP[2])+"."+std::to_string(DST_IP[3]);
        //return std::to_string(DST_IP[0])+"."+std::to_string(DST_IP[1])+"."+std::to_string(DST_IP[2])+"."+std::to_string(DST_IP[3]);
    }
    std::string get_port_str(){
        return std::to_string(DST_PORT);
    }
    void print_raw_packet(const u_char* req_raw){
        //std::cout << "|";
        for(int i = 0; i < max_length; i++){
            std::cout << (int)req_raw[i] << "|";
            if((int)req_raw[i] == 0){ // NULL end of the packet
                break;
            }
        }
        //std::cout << std::endl;
    }
public:
    enum { max_length = 8192 };
    //std::map <std::string, int> request_info;
    //std::string exec_app;
    uint VN, CD, DST_PORT;
    uint DST_IP[4];
    u_char DST_PORT_RAW[2];
    u_char DST_IP_RAW[4];
    std::string request_url;
};

void set_reply_message(u_char* buffer, int response){
    buffer[0] = (u_char)0;
    buffer[1] = (u_char)response;
}

void send_reply_message(int sockfd, int response, std::string ip, int port){//u_char high_port, u_char low_port){// std::string local_ip, std::string local_port){

        u_char buffer[8] = {0};
        buffer[0] = (u_char)0;
        buffer[1] = (u_char)response;

        // parse port
        u_char high_port = (u_char) (port / 256);
        u_char low_port = (u_char) (port % 256);
        //std::cout << "port: " << port << ", " << (int) high_port << std::endl;
        // parse ip
        std::vector<std::string> ip_tokens;
        boost::split(ip_tokens, ip, boost::is_any_of(".")); 

        if(high_port > 0 && low_port > 0){
            buffer[2] = high_port;
            buffer[3] = low_port;
            buffer[4] = (u_char) std::stoi(ip_tokens[0]);
            buffer[5] = (u_char) std::stoi(ip_tokens[1]);
            buffer[6] = (u_char) std::stoi(ip_tokens[2]);
            buffer[7] = (u_char) std::stoi(ip_tokens[3]);
        }
        //std::cout << "response: " << response << std::endl;
        //for(int i = 0; i<8; i++){
        //    std::cout << (uint)buffer[i] << "|";
        //}
        //std::cout << std::endl;
        write(sockfd, buffer, sizeof(buffer));
    }