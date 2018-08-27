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

    bool is_exist = false;
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (!strcmp(client_list[i].user_name, user_name))
        {
            close(client_list[i].num_socket);
            client_list[i].num_socket = fd_num;
            client_list[i].is_online = true;
            is_exist = true;
        };
    };

    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if ((client_list[i].num_socket == fd_num) && strcmp(client_list[i].user_name, user_name))
        {
            if (is_exist){
                client_list.erase(client_list.begin()+static_cast<long>(i));
            }else{
                strcpy(client_list[i].user_name,user_name);
                client_list[i].is_online = true;
            };
            return 0;
        };
    };
    return -1;
};

client_information* client_list::get_by_fd(int fd_num)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    for (unsigned long i=0; i< client_list.size(); i++)
    {
        if (client_list[i].num_socket == fd_num)
        { 
            return &client_list[i];
        };
    };
    return nullptr;
};

client_information* client_list::get_by_order(unsigned long order)
{
    std::lock_guard<std::mutex> guard(client_mutext);
    if (order<client_list.size())
    { 
        return &client_list[order];
    }
    else
    {
        return nullptr;
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
            client_list[i].is_online = false;
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

int client_list::is_online(int fd_num){

    client_information* client;
    client = get_by_fd(fd_num);
    if(client != nullptr)
    {
        if (client->is_online){
            return fd_num;
        }
        else
        {
            return -1;
        }
    }
    else
    {
        return -1;
    }
};

void client_list::off_client(int fd_num){
    client_information* client;
    client = get_by_fd(fd_num);
    if(client != nullptr)
    {
        client->is_online = false;
    }
};


