#include <fstream>
#include "http_parser.h"
#include "console_html_utils.h"
#include "socks4a_parser.h"
using boost::asio::ip::tcp;

class session
  : public std::enable_shared_from_this<session>
{
public:
	session(int session_id_num, tcp::socket socket, std::string file_name)
	  : socket_(std::move(socket))
	{
		session_id = "s" + std::to_string(session_id_num);
		input_file.open(file_name);
	}	
	void start()
	{
		do_read();
	}

private:
	void do_read()
	{
		auto self(shared_from_this());
		socket_.async_read_some(boost::asio::buffer(recv, max_length),
		    [this, self](boost::system::error_code ec, std::size_t length)
		    {
                //std::cout << "\ndo_read{\n" << recv <<"}do_read\n"<< std::endl;
				//std::cout.flush();
		    	if (!ec)
		    	{
					std::string received(recv), buffer;
					if(received.find("%") != std::string::npos){ // command done, ready to take next input

						if(std::getline(input_file, buffer)){
							buffer+="\n";
							memset(cmd, 0, sizeof (cmd));
							strcpy(cmd, buffer.c_str());
							output_shell(session_id, std::string(recv));
							output_command(session_id, std::string(cmd));
							do_write(buffer.length()); // you need to specify the correct length otherwise it won't work
						}

					}else{
						// TODO: print by new line????
						output_shell(session_id, std::string(recv));
						memset(recv, 0, sizeof (recv));
						do_read(); // keep reading until command done
					}
		    		
		    	}else{
					std::cout << ec.message() << std::endl;
					socket_.close();
				}
		    });
	}

	void do_write(std::size_t length)
	{
		auto self(shared_from_this());
		boost::asio::async_write(socket_, boost::asio::buffer(cmd, length),
		    [this, self](boost::system::error_code ec, std::size_t /*length*/)
		    {
		    	if (!ec)
		    	{
    	            //std::cout << "do_write{\n"<<cmd<<"\n}do_write_end\n"<< std::endl;
					memset(recv, 0, sizeof (recv));
		    		do_read();
		    	}
	   		});
  	}

	enum { max_length = 4096 };
	char cmd[max_length];
	char recv[max_length];
	tcp::socket socket_;
	std::ifstream input_file;
	std::string session_id;
};

void send_connect_request(int sockfd, std::string ip, std::string port){
	std::vector<std::string> ip_tokens;
    boost::split(ip_tokens, ip, boost::is_any_of(".")); 
	// std::cout << "req ip: " << ip << std::endl;
	u_char buffer[9] = {0};
	buffer[0] = (u_char)4;
	buffer[1] = (u_char)1;
	buffer[2] = (u_char) (std::stoi(port) >> 8);
	buffer[3] = (u_char) (std::stoi(port) % 256);
	buffer[4] = (u_char) std::stoi(ip_tokens[0]);
	buffer[5] = (u_char) std::stoi(ip_tokens[1]);
	buffer[6] = (u_char) std::stoi(ip_tokens[2]);
	buffer[7] = (u_char) std::stoi(ip_tokens[3]);
	buffer[8] = 0;

	//std::cout << "req" << std::endl;
	//for(int i = 0 ; i< 9; i++){
	//	std::cout << (int)buffer[i] << "|";
	//}std::cout << std::endl;

	write(sockfd, buffer, sizeof(buffer));

}
int main(int argc, char* argv[])
{   

	boost::asio::io_context io_context;	

	// QUERY_STRING test
	//std::string query_string = "h0=nplinux5.cs.nctu.edu.tw&p0=9847&f0=t5.txt&h1=nplinux5.cs.nctu.edu.tw&p1=9898&f1=t4.txt&h2=&p2=&f2=&h3=&p3=&f3=&h4=&p4=&f4=&sh=nplinux5.cs.nctu.edu.tw&sp=8457";

	//  get QUERY_STRING 
	char *buff = getenv("QUERY_STRING");
	std::string query_string(buff);
	// std::cout << query_string << std::endl;

	//  parse QUERY_STRING
	std::string console_html_heads, console_html_bodies;
	std::vector<boost::asio::ip::tcp::socket> sockets;
	std::vector<std::string> tokens, test_files;
    boost::split(tokens, query_string, boost::is_any_of("&"));

	// proxy information
	std::string proxy_host;
	std::string proxy_port;
	boost::asio::ip::tcp::endpoint proxy_ep;


	for(size_t i = 0; i< tokens.size(); /*i+=3*/){
		// &sh=nplinux3.cs.nctu.edu.tw&sp=12345
		if(tokens[i].find("sh") != std::string::npos){
			proxy_host = tokens[i].substr(3);
			proxy_port = tokens[i+1].substr(3);
			// DNS resolver
			boost::asio::io_service io_service;
			boost::asio::ip::tcp::resolver resolver(io_service);
			boost::asio::ip::tcp::resolver::query query(proxy_host, proxy_port);
			boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
			proxy_ep = iter->endpoint();
			break;
		}else if(tokens[i][0] == 'h'){
			i+=3;
		}
	}
	int id = 0;
	// parse remote endpoint information
	for(size_t i = 0; i< tokens.size(); /*i+=3*/){	
		// h0=nplinux1.cs.nctu.edu.tw&p0=1234&f0=t1.txt
		// parse tokens
		if(tokens[i].find("sh") != std::string::npos){
			i+=2;
		} else if(tokens[i][0] == 'h'){
			if(tokens[i].size() > 3){
				std::string raw_ip_address = tokens[i].substr(3);
				std::string port_num = tokens[i+1].substr(3);
				std::string test_file_name = "test_case/"+tokens[i+2].substr(3);
				// std::cout << "ip: " << raw_ip_address << ", port: " << port_num << ",test file name: " << test_file_name << std::endl;

				// console block html
				console_html_heads += format_head_html(raw_ip_address, port_num);
				console_html_bodies += format_body_html(i/3);

				// DNS resolver
				boost::asio::io_service io_service;
				boost::asio::ip::tcp::resolver resolver(io_service);
				boost::asio::ip::tcp::resolver::query query(raw_ip_address, port_num);
				boost::asio::ip::tcp::resolver::iterator iter = resolver.resolve(query);
				boost::asio::ip::tcp::endpoint ep = iter->endpoint();
				std::string remote_ip_num = ep.address().to_string();

				// create and connect socket to proxy server
				boost::asio::ip::tcp::socket sock(io_context, proxy_ep.protocol());
    			sock.connect(proxy_ep);
				// std::cout << sock.remote_endpoint().address().to_string() << std::endl; // you can only print remote_endpoint after connect

				// send sock4 connect request
				send_connect_request(sock.native_handle(), remote_ip_num, port_num);

				// wait for proxy server connection response
				u_char read_buffer[100];
				int len = read(sock.native_handle(), read_buffer, 8);

				// sock4 connection established, store socket and test file
				if(read_buffer[1] == REQ_GRANTED && len == 8){
					sockets.push_back(std::move(sock));
					test_files.push_back(test_file_name);
					std::make_shared<session>(id++, std::move(sockets.back()), test_files.back())->start();
				}
			}
			i+=3;
		}
	}
	// send body of the html first
	std::string main_html = build_main_html(console_html_heads, console_html_bodies);
	std::cout << "Content-type: text/html\r\n\r\n"; // http response header
	std::cout << main_html;
	std::cout.flush(); // print immediatly

	// start socket sessions together
	//for(size_t i = 0; i< sockets.size(); i++){
	//	std::make_shared<session>(i, std::move(sockets[i]), test_files[i])->start();
	//}
    
	io_context.run();
    return 0;
}