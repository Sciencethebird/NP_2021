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

namespace ip = boost::asio::ip;

struct http_parser{
public:
    http_parser(char* request_raw){

        std::string request_string(request_raw);
        
        //debug(request_string);

        // parse http package into lines
        std::vector<std::string> request_lines;
        boost::split(request_lines, request_string, boost::is_any_of("\n"));

        std::vector<std::string> tokens; // boost::split clears the vector, so you can shared the buffer

        // parse REQUEST_METHOD and SERVER_PROTOCOL
        boost::algorithm::replace_all(request_lines[0], "\r", ""); // remove return character
        boost::split(tokens, request_lines[0], boost::is_any_of(" "));
        request_info["REQUEST_METHOD"] = tokens[0];
        request_info["SERVER_PROTOCOL"] = tokens[2];

        // parse REQUEST_URI
        request_info["REQUEST_URI"] = tokens[1];

        // parse QUERY_STRING
        size_t question_mark = tokens[1].find("?");
        if(question_mark == std::string::npos){
            request_info["QUERY_STRING"] = "";
            exec_app = tokens[1];
        } else {
            request_info["QUERY_STRING"] = tokens[1].substr(question_mark+1);
            exec_app = tokens[1].substr(0, question_mark);
        }
            
        // assign HTTP_HOST
        request_info["HTTP_HOST"] = request_lines[1].substr(strlen("Host: ")); 
    }
public:
    std::map <std::string, std::string> request_info;
    std::string exec_app;
};