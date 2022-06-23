#include <sys/types.h>
#include <sys/wait.h>
#include "http_parser.h"

using boost::asio::ip::tcp;

void child_reaper(int signum){
    // SIGCLD
    while(waitpid(-1, NULL, WNOHANG)>0){}
    //debug("child exit");
}

class session
  : public std::enable_shared_from_this<session>
{
public:
    session(tcp::socket socket)
      : curr_socket(std::move(socket))
    {
        debug("new session!");
        std::cout << "new session pid: " << getpid() << std::endl; 
        counter = 0;
    }   
    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this()); // std::shared_ptr<session>
        curr_socket.async_read_some(boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length)
            {
                if (!ec)
                {
                    http_parser parser(data_);
                
                    if(parser.exec_app.find(".cgi") != std::string::npos){
                        
                        int pid = fork();

                        if(pid == 0){ //child

                            // set environment variables
                            for (const auto &item : parser.request_info) {
                                std::cout << "[" << item.first << ": " << item.second << "]\n";
                                setenv(item.first.c_str(), item.second.c_str(), 1);
                            }
                            setenv("SERVER_ADDR", curr_socket.local_endpoint().address().to_string().c_str(), 1);
                            setenv("SERVER_PORT", std::to_string(curr_socket.local_endpoint().port()).c_str(), 1);
                            setenv("REMOTE_ADDR", curr_socket.remote_endpoint().address().to_string().c_str(), 1);
                            setenv("REMOTE_PORT", std::to_string(curr_socket.remote_endpoint().port()).c_str(), 1);

                            // dup socket
                            dup2(curr_socket.native_handle(), STDIN_FILENO);
                            dup2(curr_socket.native_handle(), STDOUT_FILENO);
                            close(curr_socket.native_handle());

                            // http head
                            std::string response_head = parser.request_info["SERVER_PROTOCOL"] + " 200 OK\r\n";
                            write(STDOUT_FILENO, response_head.c_str(), response_head.size());

                            // execute the app
                            std::string exec_app = "." + parser.exec_app;
                            if (execlp(exec_app.c_str(), exec_app.c_str(),NULL) < 0){
                                fprintf(stderr, "command failed\n");
                            }
                            exit(0);
                        }else{  // parent
                            // wait????
                            close(curr_socket.native_handle());//????
                        }
                    }
                    do_read();///??????? what if you don't call this????
                }
            });
    }
    tcp::socket curr_socket;
    enum { max_length = 8192 };
    char data_[max_length];
    int counter;
};



class server
{
public:
    server(boost::asio::io_context& io_context, short port)
      : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](boost::system::error_code ec, tcp::socket socket)
            {
                if (!ec)
                {
                    std::make_shared<session>(std::move(socket))->start();
                }
                do_accept();
            });
    }   
    tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{

    signal(SIGCHLD, child_reaper);

    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: async_tcp_echo_server <port>\n";
            return 1;
        } 
        
        boost::asio::io_context io_context;   
        server s(io_context, std::atoi(argv[1])); 
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

  return 0;
}