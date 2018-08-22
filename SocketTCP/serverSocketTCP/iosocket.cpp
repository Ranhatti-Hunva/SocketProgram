#include "iosocket.h"

// using to analyser user command.
void splits_string(const std::string& subject, std::vector<std::string>& container)
{
    container.clear();
    size_t len = subject.length()+ 1;
    char* s = new char[ len ];
    memset(s, 0, len*sizeof(char));
    memcpy(s, subject.c_str(), (len - 1)*sizeof(char));
    for (char *p = strtok(s, "/"); p != nullptr; p = strtok(nullptr, "/"))
    {
        container.push_back(p);
    }
    delete[] s;
}

void send_TCP(msg_queue& msg_wts, client_list& client_socket_list, TCPserver& server_helper, bool& finish)
{
    std::vector<std::string> container;
    int socket_for_client;
    client_information client_socket_information;

    while(1)
    {
        // Read terminal non-blocking.
        fd_set reader;
        FD_SET(0,&reader);

        struct timeval terminal_tv;
        terminal_tv.tv_sec = 0;
        terminal_tv.tv_usec = 5000;

        select(1,&reader, nullptr, nullptr, &terminal_tv);
        if (FD_ISSET(0, &reader))
        {
            std::string user_cmd_str;
            getline(std::cin, user_cmd_str);
            if(!user_cmd_str.empty()){
                msg_wts.push_msg(user_cmd_str);
            };
        };

        //
        std::string msg;
        msg.clear();
        bool is_respond = false;

        if (!msg_wts.respond_empty()){
            is_respond = true;
            msg = msg_wts.respond_get();
        } else if(!msg_wts.msg_empty()) {
            is_respond = false;
            msg = msg_wts.msg_get();
        }

        // Send message
        if (!msg.empty())
        {            
            // Server close echo-socket.
            if (!msg.compare("#"))
            {
                finish = true;
                break;
            }
            else
            {
                splits_string(msg, container);
                // Does msg have 2 part?
                if(container.size() == 2)
                {
                    // Is part 2 is a number.
                    bool isNumber = true;
                    try
                    {
                        socket_for_client = stoi(container[1]);
                    }
                    catch(...)
                    {
                        printf("=>The folowing text is no a number !! \n");
                        isNumber = false;
                    };

                    if (isNumber)
                    {
                        // Searching client.
                        if(client_socket_list.get_by_fd(socket_for_client, client_socket_information) < 0)
                        {
                            printf("=>Don't have this socket in the list \n");
                        }
                        else
                        {
                            // Force close connection with client by server.
                            if (!container[0].compare("#"))
                            {
                                server_helper.closer(socket_for_client,client_socket_list);
                                printf("=> Closed connection with soket %d.\n",socket_for_client);
                            }
                            else // Send msg to client.
                            {
                                if(!server_helper.send_msg(socket_for_client, container[0], is_respond))
                                {
                                    printf("=> Error on scket %d.\n",socket_for_client);
                                    server_helper.closer(socket_for_client,client_socket_list);
                                };
                            };
                        };
                    };
                }
                else
                {
                    printf("=> Wrong format message or no massge to send !! \n");
                };
            };
            (is_respond)?msg_wts.pop_respond():msg_wts.pop_msg();
        }
        else
        {
            sleep(1);
        };
    };
};

void process_on_buffer_recv(const char* buffer, client_list& client_socket_list, int client_fd, msg_queue& msg_wts)
{
    std::string bufstr = buffer;
    std::vector<std::string> container;
    std::string message;

    if (bufstr.compare("#"))
    {
        splits_string(bufstr, container);

        if(container.size() == 2)
        {
            if(!container[0].compare("*"))
            {
                if(client_socket_list.set_user_name(client_fd,container[1].c_str()) < 0)
                {
                    printf("=> Error on inserting username for new socket!! \n");
                };
            }
            else
            {
                // Format message
                int forward_fd = client_socket_list.get_fd_by_user_name(container[1].c_str());
                forward_fd = client_socket_list.is_online(forward_fd);

                if (forward_fd < 0)
                {
                    message = "Sorry,"+container[1]+" hasn't connect to server!!"+"/"+std::to_string(client_fd);
                }
                else
                {
                    client_information host_msg;
                    client_socket_list.get_by_fd(client_fd, host_msg);

                    std::string str(host_msg.user_name);
                    message = str+"=>"+container[0]+"/"+std::to_string(forward_fd);
                }

                // Notify send_TCP wake up to send data.
                msg_wts.push_msg(message);
            };
        }
        else
        {
            printf("=> Wrong format stype recived message \n");
        };
    };
};
