.PHONY: all
CXX=g++
CXXFLAGS=-std=c++11 -Wall -pedantic -pthread -lboost_system
CXX_INCLUDE_DIRS=/usr/local/include
CXX_INCLUDE_PARAMS=$(addprefix -I , $(CXX_INCLUDE_DIRS))
CXX_LIB_DIRS=/usr/local/lib
CXX_LIB_PARAMS=$(addprefix -L , $(CXX_LIB_DIRS))
all: 
	$(CXX) socks_server.cpp -o socks_server $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
	$(CXX) console.cpp -o hw4.cgi $(CXX_INCLUDE_PARAMS) $(CXX_LIB_PARAMS) $(CXXFLAGS)
pack:
	rm -rf 310605004
	mkdir 310605004
	cp console.cpp 310605004/console.cpp
	cp http_parser.h 310605004/http_parser.h
	cp console_html_utils.h 310605004/console_html_utils.h
	cp socks4a_parser.h 310605004/socks4a_parser.h
	cp socks_server.cpp 310605004/socks_server.cpp
	cp Makefile 310605004/Makefile
test_sock4a:
	# curl --socks4a <host[:port]> <URL>
	curl --socks4a 127.0.0.1:7777 ppp
move:
	chmod +x hw4.cgi
	cp hw4.cgi ../public_html/
	
clean:
	rm -f proxy_server hw4.cgi