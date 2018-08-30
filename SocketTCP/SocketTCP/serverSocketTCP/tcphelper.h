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
#include <chrono>
#include <fcntl.h>

#include "clientmanager.h"
#include "msgqueue.h"
#include "threadpool.h"

#define SGI 0
#define MSG 1
#define PIG 2
#define RSP 3

class TCPhelper {
protected:
    std::mutex fd_set_mutex;
    fd_set master;
    int fd_max;

    const int timeout = 5;
public:
    struct timeval general_tv;

    struct rps_timeout{
        msg_text msg;
        std::chrono::system_clock::time_point timeout;
        int socket;
    };

    std::vector<rps_timeout> rps_timeout_list;

    TCPhelper();
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Packed msg ad format.
    bool packed_msg(const msg_text msg_input, const int socket_fd, q_element& element);

    // Unpacked msg.
     bool unpacked_msg(msg_text& msg, const std::vector<unsigned char> buffer);

    // Get message confirm
    void msg_confirm(const std::string rps);
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
    int reciver(int server_fd, client_list& client_socket_list,msg_queue& msg_wts, thread_pool& threads);

    // Analyzer buffer, ....
    void process_on_buffer_recv(const unsigned char buffer[], const long num_data, client_list& client_socket_list, const int socket_fd, msg_queue& msg_wts);

    // Send packed message.
    void send_msg(msg_queue& msg_wts, bool& end_connection, client_list& client_socket_list);

    // Close socket.
    void closer(int server_fd, client_list& client_socket_list);

    // Timeout locker
    void timeout_clocker(bool& end_connection, client_list& client_socket_list);

    void hello(int& a);
};

void ultoc(unsigned int& ul, unsigned char* cu);

#endif // TCPHELPER_H
