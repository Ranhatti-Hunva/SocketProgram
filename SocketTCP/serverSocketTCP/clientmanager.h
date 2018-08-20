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

struct  client_information
{
    char user_name[256];
    int num_socket;
//    std::queue<std::string> msg_queue;
    std::string msg_incompleted;
};

class client_list{
    std::vector <client_information> client_list;
    std::mutex client_mutext;

    int remove_by_order(unsigned long order);
public:
    void add_fd(int fd_num);

    int set_user_name(int fd_num, const char* user_name);

    int get_by_fd(int fd_num, client_information& contain_information);

    int delete_fs_num(int fd_num);

    int get_by_order(unsigned long order,  client_information& contain_information);

    int get_fd_by_user_name(const char* user_name);

    unsigned long size();
};

#endif // CLIENTMANAGER_H
