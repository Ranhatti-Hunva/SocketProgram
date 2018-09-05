#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <arpa/inet.h>
#include <vector>
#include <mutex>
#include <string.h>
#include <string>
#include <queue>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>

//#include "tcphelper.h"

struct  client_information
{
    char user_name[256];
    int socket_fd;    
    bool is_online;

    std::vector<unsigned char> buffer;
};

class client_list{
    std::vector <client_information> client_list;
    std::mutex client_mutext;

public:
    std::mutex buffer_mutex;
    void add_fd(int fd_num);
    int set_user_name(int fd_num, const char* user_name);

    client_information* get_by_fd(int fd_num);
    client_information* get_by_order(unsigned long order);

    int get_fd_by_user_name(const char* user_name);

    int is_online(int fd_num);
    void off_client(int fd_num);

    unsigned long size();
};

#endif // CLIENTMANAGER_H
