# TCP-IP_Chat
A server-client model using TCP/IP socket programming for a simple chat application.

# Compilation
1. client.c : gcc client.c -lpthread -o client
2. server.c : gcc server.c -lpthread -o server  `mysql_config --cflags --libs` 

# Execution
1. client : client <ip address> <port>
2. server : server <port>
