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

//#include "clientmanager.h"
#include "iosocket.h"

class TCPhelper
{    
protected:
    fd_set master;
    int fd_max;

    struct timeval general_tv;

    static const unsigned int bufsize = 256;
public:
    TCPhelper();
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Send packed message.
    int send_pk_msg(int fd, const char* msg);
};

class TCPclient: public TCPhelper{
public:
    std::string msg_incomplete;

    TCPclient():TCPhelper(){}

    // Creat new socket and connect to a server with timeout. Let decision to re-connect on user.
    int connect_with_timeout(struct addrinfo *server_infor);

    // Recive raw message on client.
    // Return number bytes recived and pass message to msg pointer if success.
    // Return -1 if connecton fail or disconnect.
    int recv_msg(int fd);
};


class TCPserver: public TCPhelper{
    // Make a server socket TCP/IP
    // int server_echo(int port_num);

    // Acceptor for a new client connection
    // int acceptor(int server_fd, std::vector<int>& input_fds, fd_set& master,int& fdmax, client_list& client_socket_list);
};

#endif // TCPHELPER_H
