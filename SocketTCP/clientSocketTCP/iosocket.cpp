#include "iosocket.h"
#include "tcphelper.h"

//void send_TCP(user_command& user_command, fd_set& master, int& socket_fd)
void send_TCP(msg_queue& msg_wts, TCPclient& client_helper, int& socket_fd, bool& end_connection)
{
    std::string user_cmd_str;
    user_cmd_str.clear();

    bool is_login = true;
    while(!end_connection)
    {
        if (is_login)
        {
            // If just login, send a msg as format */+user_name
            msg_wts.push("*/"+user_name);
            is_login = false;
        }
        else
        {
            // Get unbocking user input from terminal.
            fd_set reader;
            FD_SET(0,&reader);

            struct timeval general_tv;
            general_tv.tv_sec = 0;
            general_tv.tv_usec = 5000;

            select(1,&reader, nullptr, nullptr, &general_tv);
            if (FD_ISSET(0, &reader))
            {
                getline(std::cin, user_cmd_str);
                msg_wts.push(user_cmd_str);
            }
            else
            {
                user_cmd_str.clear();
            };
        };

        if (!msg_wts.empty())
        {
            if(msg_wts.get().compare("#"))
            {
                // Send msg.
                if(!client_helper.send_msg(socket_fd,msg_wts.get()))
                {
                    printf("=> Sending failure !!!");
                    // client_helper.pinger(socket_fd);
                }
                else
                {
                    msg_wts.pop();
                };
            }
            else
            {
                // Exit thread.
                end_connection = true;
                break;
            };
        }
        else
        {
            sleep(1);
        };
    };
};

int is_reconnect(int& client_fd)
{
    // Ask user if they want to reconnect with server after lose connection.
    std::string answer;
    while (1)
    {
        printf("=> Do you want reconnect to server (Y/N)?\n");
        getline(std::cin, answer);

        if(!answer.compare("Y"))
        {
            close(client_fd);
            return 0;
        }
        else if (!answer.compare("N"))
        {
            close(client_fd);
            return -1;
        }
        else
        {
            printf("=> Don't understand the answer.\n");
            continue;
        }
    };
};








