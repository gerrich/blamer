all: tcp_sync_echo_server

tcp_sync_echo_server:
	g++ -O0 -g tcp_sync_echo_server.cpp -o tcp_sync_echo_server -L /usr/lib/x86_64-linux-gnu/ -lboost_system

clear:
	rm -rf tcp_sync_echo_server
