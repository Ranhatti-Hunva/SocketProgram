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
#include "msgqueue.h"

class TCPhelper {
protected:
    std::mutex fd_set_mutex;
    fd_set master;
    int fd_max;

    struct timeval general_tv;

    static const unsigned int bufsize = 256;
public:
    TCPhelper();
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Packed msg ad format <2(char)><msg><3(char)> (2 is Start of Text, 3 is End of Text in ASCII).
    void packed_msg(std::string& msg);

    // Unpacked msg.
    bool unpacked_msg(char* buffer, std::string& msg_incomplete);

    // Send packed message.
    bool send_msg(int fd, std::string msg);
};

class TCPserver:public TCPhelper
{
    std::vector<int> client_fds;
public:
    TCPserver():TCPhelper()
    {
        client_fds.clear();
    }
    // Make a server socket TCP/IP
    int server_echo(int port_num);

    // Acceptor for a new client connection
    int acceptor(int server_fd, client_list& client_socket_list);

    // Reciver for a msg from client.
    int reciver(int server_fd, client_list& client_socket_list,msg_queue& msg_wts);

    // Close socket.
    void closer(int server_fd, client_list& client_socket_list);
};

#endif // TCPHELPER_H
