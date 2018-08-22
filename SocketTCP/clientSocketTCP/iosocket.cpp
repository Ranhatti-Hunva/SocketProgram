#include "iosocket.h"
#include "tcphelper.h"

//void send_TCP(user_command& user_command, fd_set& master, int& socket_fd)
void send_TCP(msg_queue& msg_wts, TCPclient& client_helper, int& socket_fd, bool& end_connection)
{
    // If just login, send a msg as format */+user_name
    msg_wts.push_msg("*/"+user_name);

    std::string user_cmd_str;
    user_cmd_str.clear();

    while(!end_connection)
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
            if(!user_cmd_str.empty())
            {
                msg_wts.push_msg(user_cmd_str);
            };
        };

        bool is_respond = false;
        if(false == client_helper.ping)
        {
            std::string msg;
            msg.clear();

            if (!msg_wts.respond_empty())
            {
                is_respond = true;
                msg = msg_wts.respond_get();
            }
            else if (!msg_wts.msg_empty())
            {
                is_respond = false;
                msg = msg_wts.msg_get();
            };

            if (!msg.empty())
            {
                if(msg.compare("#"))
                {
                    // Send msg.
                    if(!client_helper.send_msg(socket_fd, msg, client_helper.rps_timeout_list, is_respond))
                    {
                        printf("=> Sending failure !!!");
                        // Exit thread.
                        end_connection = true;
                        break;
                    }
                    else
                    {
                        (is_respond)?msg_wts.pop_respond():msg_wts.pop_msg();
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
        }
        else
        {
            // Send ping;
            if(!client_helper.send_msg(socket_fd, "PING", client_helper.rps_timeout_list, is_respond))
            {
                printf("=> Sending failure !!!");
                // Exit thread.
                end_connection = true;
                break;
            };
            client_helper.ping_msg_ID = client_helper.rps_timeout_list.back().ID;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
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









