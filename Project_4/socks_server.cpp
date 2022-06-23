#include <sys/types.h>
#include <sys/wait.h>
#include <fstream>
#include <iostream>
#include <string>
#include "http_parser.h"
#include "socks4a_parser.h"
using boost::asio::ip::tcp;

void child_reaper(int signum){
    // SIGCLD
    while(waitpid(-1, NULL, WNOHANG)>0){}
    //debug("child exit");
}

class proxy_tunnel
  : public std::enable_shared_from_this<proxy_tunnel>
{
public:
    proxy_tunnel(tcp::socket client_sock, tcp::socket remote_sock)
      : client_socket(std::move(client_sock)), remote_socket(std::move(remote_sock))
    {
        //debug("new session!");
        //std::cout << "new session pid: " << getpid() << std::endl; 
        pid = getpid();
        counter = 0;
    }   
    void start()
    {
        do_read_client();
        do_read_remote();
    }

private:
    void do_read_client()
    {
        auto self(shared_from_this()); // std::shared_ptr<session>
        client_socket.async_read_some(boost::asio::buffer(client_to_remote_buffer, max_length),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                //std::string tmp(data_);
                if (!ec)
                {
                    //std::cout <<"["<< pid << "] "<<"client async_read, size:" << std::string(client_to_remote_buffer).size() << "\nbuffer: " << std::string(client_to_remote_buffer) << std::endl;
                    // after reading some message from client, send to remote
                    do_write_remote(length);
                }
                //std::cout << "error!!!!!: " << ec << std::endl;
            });
    }
    void do_write_remote(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(remote_socket, boost::asio::buffer(client_to_remote_buffer, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    memset(client_to_remote_buffer, 0, max_length);
                    do_read_client();
                }
            });
    }
    void do_read_remote()
    {
        auto self(shared_from_this()); // std::shared_ptr<session>
        remote_socket.async_read_some(boost::asio::buffer(remote_to_client_buffer, max_length),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                //std::string tmp(data_);
                if (!ec)
                {
                    //sock4a_request_parser req(read_buffer);
                    // <<"["<< pid << "] "<< "remote async_read \n size: "<<std::string(remote_to_client_buffer).size() <<  "\nbuffer: " << std::string(remote_to_client_buffer) << std::endl;

                    //do_read();///??????? what if you don't call this????
                    do_write_client(length);
                }
            });
    }
    
    void do_write_client(std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(client_socket, boost::asio::buffer(remote_to_client_buffer, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    memset(remote_to_client_buffer, 0, max_length);
                    do_read_remote();
                }
            });
    }
private:
    tcp::socket client_socket;
    tcp::socket remote_socket;
    enum { max_length = 1000000 };
    char client_to_remote_buffer[max_length]; // unsigned char???????
    char remote_to_client_buffer[max_length]; // unsigned char???????
    //char write_buffer[8]; // sock4a reply
    int counter;
    int pid;
};


class server
{
public:
    server(boost::asio::io_context& io_context, short port, std::string conf)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)), conf_file(conf)
    {
        // start accept
        do_accept(io_context);
    }
private:
    void update_config(){
        connect_ip_masks.clear();
        bind_ip_masks.clear();
        std::ifstream file;
        file.open(conf_file);
        std::string buffer;
        while(std::getline(file, buffer)){
            if(buffer.find("permit c") != std::string::npos){
                connect_ip_masks.push_back(buffer.substr(9));
                //std::cout << connect_ip_masks.back() << std::endl;
            } else if (buffer.find("permit b") != std::string::npos){
                bind_ip_masks.push_back(buffer.substr(9));
                //std::cout << bind_ip_masks.back() << std::endl;
            }
        }
        file.close();
    }
    bool is_legal_ip(std::string ip, int mode){
        update_config();
        bool legal = false;
        if(mode == CONNECT){
            for(auto mask: connect_ip_masks){
                for(ulong i = 0; i < ip.size(); i++){
                    if(ip[i] != mask[i]){
                        //std::cout << "connect unmatched char: " << mask[i] << std::endl;
                        if(mask[i] == '*'){
                            legal = true;
                        }
                        break;
                    }
                }
                if(legal) break;
            }
        } else {
            for(auto mask: bind_ip_masks){
                for(ulong i = 0; i < ip.size(); i++){
                    if(ip[i] != mask[i]){
                        //std::cout << "bind unmatched char: " << mask[i] << std::endl;
                        if(mask[i] == '*'){
                            legal = true;
                        }
                        break;
                    }
                }
                if(legal) break;
            }
        }
        //std::cout << "legal: " << legal <<std::endl;
        return legal;
    }
private:
    void do_accept(boost::asio::io_context& io_context)
    {
        acceptor_.async_accept(
            [this, &io_context](boost::system::error_code ec, tcp::socket client_socket)
            {
                if (!ec)
                {
                    //std::make_shared<session>(std::move(socket))->start();
                    // socks4 connection handler
                    //int pid = fork();
                    if(fork() == 0){
                        io_context.notify_fork(boost::asio::io_context::fork_child);
                        //std::cout << "in child" << std::endl;
                        read(client_socket.native_handle(), read_buffer, max_length);
                        //std::cout << "buffer" << (int)read_buffer[0] << std::endl;
                        sock4a_request_parser req(read_buffer);

                        // clean message buffer
                        memset(read_buffer, 0, sizeof (read_buffer)); // avoid dirty data buffer???
                        //send_reply_message(socket.native_handle(), REQ_DENIED);
                        
                        // client port and address
                        std::string source_ip = client_socket.remote_endpoint().address().to_string();
                        int source_port = client_socket.remote_endpoint().port();
                        
                        if(req.VN == 4){ // for sock4a only
                            // DNS resolver
                            //std::cout << "Destination IP: " << req.get_ip_str() << " port:" << req.get_port_str() <<", bind: "<< req.CD << std::endl;
			                boost::asio::io_service io_service;
			                boost::asio::ip::tcp::resolver resolver(io_service);
			                boost::asio::ip::tcp::resolver::query query(req.get_ip_str(), req.get_port_str());
			                boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			                boost::asio::ip::tcp::endpoint ep = iter->endpoint();

                            if(req.CD == CONNECT){
                                
                                // create and connect socket
			                    boost::asio::ip::tcp::socket remote_socket(io_context, ep.protocol());
    		                    
                                //std::cout << "CONNECT to: " << ep.address().to_string() << std::endl;
                                if(is_legal_ip(ep.address().to_string(), CONNECT)){
                                    remote_socket.connect(ep); // ???????????????????
                                    send_reply_message(client_socket.native_handle(), REQ_GRANTED, "0.0.0.0", 0);
                                    std::make_shared<proxy_tunnel>(std::move(client_socket), std::move(remote_socket))->start();
                                    req.print_request(source_ip, source_port, REQ_GRANTED);
                                } else{
                                    send_reply_message(client_socket.native_handle(), REQ_DENIED, "0.0.0.0", 0);
                                    req.print_request(source_ip, source_port, REQ_DENIED);
                                }
                                //send_reply_message(client_socket.native_handle(), REQ_GRANTED);
                                //set_reply_message(write_buffer, REQ_GRANTED);
                            } else if(req.CD == BIND) {
                                
                                if(is_legal_ip(ep.address().to_string(), BIND)){
                                    //std::cout << "server IP: " << client_socket.local_endpoint().address().to_string().c_str()  << std::endl;
                                    // << "server PORT: " << std::to_string(client_socket.local_endpoint().port()).c_str()  << std::endl;

                                    boost::asio::ip::tcp::endpoint this_host(boost::asio::ip::tcp::v4(), 0);
                                    boost::asio::ip::tcp::socket remote_socket(io_context);
                                    ip::tcp::acceptor binder(io_service, this_host);

                                    //std::cout << "bind IP: " << binder.local_endpoint().address().to_string().c_str()  << std::endl;
                                    //std::cout << "bind PORT: " << std::to_string(binder.local_endpoint().port()).c_str()  << std::endl;

                                    std::string local_ip = binder.local_endpoint().address().to_string();
                                    int local_port = binder.local_endpoint().port();

                                    send_reply_message(client_socket.native_handle(), REQ_GRANTED, local_ip, local_port);
                                    binder.accept(remote_socket);
                                    //std::cout << "remote IP: " << binder.local_endpoint().address().to_string().c_str()  << std::endl;
                                    //std::cout << "remote PORT: " << std::to_string(binder.local_endpoint().port()).c_str()  << std::endl;
                                    send_reply_message(client_socket.native_handle(), REQ_GRANTED, local_ip, local_port);
                                    std::make_shared<proxy_tunnel>(std::move(client_socket), std::move(remote_socket))->start();
                                    req.print_request(source_ip, source_port, REQ_GRANTED);

                                } else{
                                    send_reply_message(client_socket.native_handle(), REQ_DENIED, "0.0.0.0", 0);
                                    req.print_request(source_ip, source_port, REQ_DENIED);
                                }

                                
                            } else {
                                send_reply_message(client_socket.native_handle(), REQ_DENIED, 0, 0);
                                req.print_request(source_ip, source_port, REQ_DENIED);
                                //set_reply_message(write_buffer, REQ_DENIED);
                            }
                            
                        }     
                        // TODO !!!!!!!!!!!!!
                        // print out message!!!!

                    }else{ // parent
                        io_context.notify_fork(boost::asio::io_context::fork_parent);
                        // close(client_socket.native_handle()); 
                        // I don't know why but if you close this socket may cause following problem
                        // Exception: epoll re-registration: File exists
                        do_accept(io_context); // do this in else makes much more sense
                    }
                }
                
            });
    }   
    tcp::acceptor acceptor_;
private:
    enum { max_length = 8192 };
    u_char read_buffer[max_length]; // unsigned char???????
    std::string conf_file;
    std::vector<std::string> connect_ip_masks;
    std::vector<std::string> bind_ip_masks;
};

int main(int argc, char* argv[])
{
    signal(SIGCHLD, child_reaper);
    std::string sock_conf_file = "socks.conf";
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        } 
        
        boost::asio::io_context io_context;   
        server s(io_context, std::atoi(argv[1]), sock_conf_file); 
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

  return 0;
}