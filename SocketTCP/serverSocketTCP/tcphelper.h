#ifndef TCPHELPER_H
#define TCPHELPER_H

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <vector>
#include <fcntl.h>

#include "clientmanager.h"

class TCPhelper
{
    struct addrinfor* my_addr;
public:
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Make a server socket TCP/IP
    int server_echo(int port_num);

    // Acceptor for a new client connection
    int acceptor(int server_fd, std::vector<int>& input_fds, fd_set& master,int& fdmax, client_list& client_socket_list);

    // Creat new socket fd
    int create_master_server(int doimain, int type, int protocol);

    // Connect to a server with timeout. Let decision to re-connect on user.
    int connect_with_timeout(int socket_fd, struct addrinfo* des_addr, struct timeval tv);

    // Make a client socket TCP/IP
    int client(std::string host_name);


};

#endif // TCPHELPER_H
