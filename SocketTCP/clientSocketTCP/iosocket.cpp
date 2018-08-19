#include "iosocket.h"

void send_TCP(user_command& user_command, fd_set& master, int& socket_fd)
{
    fd_set send_fds;
    FD_ZERO(&send_fds);

    struct timeval general_tv;
    general_tv.tv_sec = 0;
    general_tv.tv_usec = 5000;

    std::string user_cmd_str;
    bool is_login = true;

    while(1)
    {
        if (is_login)
        {
            user_cmd_str = "*/"+user_name;
            is_login = false;
        }
        else
        {
            if(user_command.compare("#"))
            {
                break;
            }
            else
            {
                fd_set reader;
                FD_SET(0,&reader);
                select(1,&reader, nullptr, nullptr, &general_tv);
                if (FD_ISSET(0, &reader))
                {
                    getline(std::cin, user_cmd_str);
                    user_command.set(user_cmd_str);
                }
                else
                {
                    user_cmd_str.clear();
                };
            };
        };

        if (!user_cmd_str.empty())
        {
            if(user_cmd_str.compare("#"))
            {
                const unsigned int send_bufsize = 256;
                char send_buffer[send_bufsize];
                memset(&send_buffer,0,send_bufsize/sizeof(char));

                strcpy(send_buffer, user_cmd_str.c_str());

                unsigned long total_bytes, byte_left, sended_bytes;
                total_bytes = user_cmd_str.length();
                byte_left = total_bytes;
                sended_bytes = 0;
                int try_times = 0;

                while(sended_bytes < total_bytes)
                {
                    send_fds = master;
                    if (select(socket_fd+1,nullptr, &send_fds, nullptr, &general_tv) <0)
                    {
                        perror("=>Select ");
                        exit(EXIT_FAILURE);
                    };

                    if(FD_ISSET(socket_fd, &send_fds))
                    {
                        long status = send(socket_fd, send_buffer+sended_bytes, byte_left, 0);
                        if (status < 0)
                        {
                            printf("=> Sending failure !!!");
                            if (try_times++ < 3)
                            {
                                break;
                            };
                        }
                        else
                        {
                            sended_bytes += static_cast<unsigned long>(status);
                            byte_left -= static_cast<unsigned long>(status);
                        };
                    }
                    else
                    {
                        printf("=> Socket is not ready to send data!! \n");
                        if (try_times++ < 3)
                        {
                            printf("=> Error on sending message");
                            break;
                        };
                    };
                };
            }
            else
            {
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


bool unpacked_msg(char* buffer, std::string& msg_incomplete)
{
    char* p = buffer;
    while(*p != 0)
    {
        switch (*p)
        {
            case 2:
            {
                msg_incomplete.clear();
                break;
            };
            case 3:
            {
                return true;
            };
            default:
            {
                msg_incomplete.append(p);
                break;
            };
        };
        p++;
    };

    return false;
}







