#ifndef UDPHELPER_H
#define UDPHELPER_H

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

// Time function, sockets, htons... file stat
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/stat.h>

// File function and bzero
#include <fcntl.h>
#include <strings.h>

//class UDPhelper
//{
//    char ipstr[INET_ADDRSTRLEN];
//    off_t total_byte;
//    std::string name_of_file;

//    static const int bufsize = 1024;
//public:
//    struct addrinfo* get_addinfo_list(std::string host_name, int port_num);

//    // Listerner
////    void operator()(int size_of_file);

//    // Talker
//    void operator()(std::string& UDP_addr,std::string filename);
//};

#endif // UDPHELPER_H
