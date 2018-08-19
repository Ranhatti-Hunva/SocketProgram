#include "clientmanager.h"

void client_list::add_fd( int fd_num)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    client_information client_element;
    client_element.num_socket = fd_num;
    client_list.push_back(client_element);
};

int client_list::set_user_name(int fd_num, const char* user_name)
{
    std::lock_guard<std::mutex> guard(client_mutext);

    bool is_exit = false;

    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (!strcmp(client_list[i].user_name, user_name))
        {
            close(client_list[i].num_socket);
            client_list[i].num_socket = fd_num;
            is_exit = true;
        };
    };


    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == fd_num)
        {
            if (is_exit){
                client_list.erase(client_list.begin()+static_cast<long>(i));
            }else{
                strcpy(client_list[i].user_name,user_name);
            };
            return 0;
        };
    };
    return -1;
};

int client_list::get_by_fd(int fd_num, client_information& contain_information)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == fd_num)
        {
            contain_information = client_list[i];
            return 0;
        };
    };
    return -1;
};

int client_list::get_by_order(unsigned long order,  client_information& contain_information)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    if ((order>0) && (order<client_list.size()))
    {
        contain_information =  client_list[order];
        return 0;
    }
    else
    {
        return -1;
    }
};



int client_list::delete_fs_num(int fd_num)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == fd_num)
        {
            client_list[i].num_socket = -1;
            return 0;
        };
    };
    return -1;
};

unsigned long client_list::size()
{
    std::lock_guard<std::mutex> guard(client_mutext);
    return client_list.size();
};

int client_list::remove_by_order(unsigned long order)
{
    if ((order>0) && (order<client_list.size()))
    {
        client_list.erase(client_list.begin()+static_cast<long>(order));
        return 0;
    }
    else
    {
        return -1;
    }
};

int client_list::get_fd_by_user_name(const char* user_name)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (!strcmp(client_list[i].user_name,user_name))
        {
            return client_list[i].num_socket;
        };
    };
    return -1;
};


