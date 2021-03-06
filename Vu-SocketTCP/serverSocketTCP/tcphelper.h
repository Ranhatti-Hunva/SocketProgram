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

    const int timeout = 10;
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
    bool packed_msg(msg_text& msg_input, std::vector<unsigned char>& element);

    // Unpacked msg.
     bool unpacked_msg(msg_text& msg_output, std::vector<unsigned char>& buffer);

    // Get message confirm
    void msg_confirm(const msg_text rsp);
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
    int recv_msg(int server_fd, client_list& client_socket_list, msg_queue& msg_wts, thread_pool& threads);

    // Analyzer buffer, ....
    void buffer_analyser(bool& end_connection, msg_queue& msg_wts, client_list& client_socket_list, thread_pool& threads);
    void process_on_buffer_recv(const msg_text msg_get, client_list& client_socket_list, const client_information* host_msg, msg_queue& msg_wts);

    // Send packed message.
    void send_msg(msg_queue& msg_wts, bool& end_connection, client_list& client_socket_list);

    // Post_sen_process;
    void post_send_process(q_element element, const bool is_rsp, msg_queue& msg_wts);

    // Close socket.
    void closer(int server_fd, client_list& client_socket_list);

    // Get timeout of msg.
    void timeout_clocker(bool& end_connection, client_list& client_socket_list);
};

void ultoc(unsigned int& ul, unsigned char* cu);

#endif // TCPHELPER_H
