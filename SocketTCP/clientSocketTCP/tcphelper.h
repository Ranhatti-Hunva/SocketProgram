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

class TCPhelper
{    
protected:
    fd_set master;
    int fd_max;

    struct timeval general_tv;

    static const unsigned int bufsize = 256;

    struct rps_timeout{
        char ID;
        std::chrono::system_clock::time_point timeout;
        int socket;
    };

public:
    TCPhelper();
    // Get address information from host name.
    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

    // Packed msg ad format <2(char)><msg><3(char)> (2 is Start of Text, 3 is End of Text in ASCII).
    char packed_msg(std::string& msg);

    // Unpacked msg.
    bool unpacked_msg(char* buffer, std::string& msg_incomplete, char& ID_msg_incomplete);

    // Send packed message.
    bool send_msg(int fd, std::string msg, std::vector<rps_timeout>& rps_queue_timeout, bool& is_rps);
};

class TCPclient: public TCPhelper{
public:
    std::string msg_incomplete;
    char ID_msg_incomplete;

    std::vector<rps_timeout> rps_queue_timeout;

    TCPclient():TCPhelper(){}

    // Creat new socket and connect to a server with timeout. Let decision to re-connect on user.
    // If fail by error socket or timeout, return -1;
    // If success return client_fd > 0;
    int connect_with_timeout(struct addrinfo *server_infor);

    // Recive and unpaccked message.
    // Return 1 if message is gotten completely. An then get msg in msg_incomplete of the object.
    // Return -1 if connecton fail or disconnect from server.
    // Return 0 if just got a part of message.
    int recv_msg(int fd);

    // Ping when error in reponds timeout.
    bool pinger(int fd);

    void timeout_clock();

    void msg_confirm(const std::string rps);
};

#endif // TCPHELPER_H
