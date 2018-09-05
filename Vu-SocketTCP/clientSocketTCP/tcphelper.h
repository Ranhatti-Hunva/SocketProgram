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
#include <chrono>
#include <queue>

#include "msgqueue.h"
#include "threadpool.h"

#define SGI 0
#define MSG 1
#define PIG 2
#define RSP 3

static std::mutex log_mutext;

class TCPhelper
{    
protected:
    fd_set master;
    int fd_max;

    struct timeval general_tv;

    const int timeout = 5;

public:
    struct rps_timeout{
        msg_text msg;
        std::chrono::system_clock::time_point timeout;
        int socket;
    };

    std::vector<rps_timeout> rps_timeout_list;

    TCPhelper();
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Packed msg ad format <2(char)><msg><3(char)> (2 is Start of Text, 3 is End of Text in ASCII).
    bool packed_msg(msg_text& msg_input, std::vector<unsigned char>& element);

    // Unpacked msg.
    bool unpacked_msg(msg_text& msg_output, std::vector<unsigned char>& buffer);

    // Get msg confirm
    void msg_confirm(const msg_text rsp);
};

class TCPclient: public TCPhelper{
public:
    std::vector<unsigned char> buffer;
    std::mutex ping_mutex;
    std::condition_variable con_ping;

    bool ping;
    msg_text ping_msg;

    TCPclient():TCPhelper()
    {
        ping = false;
        rps_timeout_list.clear();
        ping_msg.type_msg = PIG;
    }

    std::mutex buffer_mutex;

    // Creat new socket and connect to a server with timeout. Let decision to re-connect on user.
    int connect_with_timeout(struct addrinfo *server_infor);

    // Recive and unpaccked message.
    int recv_msg(const int socket_fd, msg_queue& msg_wts, thread_pool& thread);

    // Send packed message.
    void send_msg(msg_queue& msg_wts, bool& end_connection, int socket_fd);

    void process_on_buffer_recv(const unsigned char buffer[], const long num_data, msg_queue& msg_wts);

    void timeout_clocker(bool& end_connection);
};

void ultoc(unsigned int& ul, unsigned char* cu);

#endif // TCPHELPER_H
