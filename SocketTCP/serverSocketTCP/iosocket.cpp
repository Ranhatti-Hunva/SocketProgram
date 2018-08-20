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
            msg_wts.push(user_cmd_str);
        };

        // Send message
        if (!msg_wts.empty())
        {            
            // Server close echo-socket.
            if (!msg_wts.get().compare("#"))
            {
                finish = true;
                break;
            }
            else
            {
                splits_string(msg_wts.get(), container);
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
//                            std::unique_lock<std::mutex> locker_fd_set(fd_set_muxtex,std::defer_lock);
                            // Force close connection with client by server.
                            if (!container[0].compare("#"))
                            {
                                server_helper.closer(socket_for_client,client_socket_list);

//                                locker_fd_set.lock();
//                                FD_CLR(socket_for_client, &master);
//                                client_socket_list.delete_fs_num(socket_for_client);    // delete client information.
//                                close(socket_for_client);
//                                input_fds.erase(std::remove(input_fds.begin(), input_fds.end(), socket_for_client), input_fds.end());
//                                locker_fd_set.unlock();

                                printf("=> Closed connection with soket %d.\n",socket_for_client);
                            }
                            else // Send msg to client.
                            {
                                // Return ??
                                server_helper.send_msg(socket_for_client, container[0]);
//                                server_helper.packed_msg(container[0]);

//                                memset(&buffer,0,bufsize/sizeof(char));
//                                strcpy(buffer, container[0].c_str());
//                                total_bytes = container[0].length();

//                                byte_left = total_bytes;
//                                sended_bytes = 0;
//                                int try_times = 0;

//                                locker_fd_set.lock();
//                                while(sended_bytes < total_bytes)
//                                {
//                                    send_fds = master;
//                                    if (select(fdmax+1,nullptr, &send_fds, nullptr, &tv) <0)
//                                    {
//                                        perror("=>Select ");
//                                        exit(EXIT_FAILURE);
//                                    };

//                                    if(FD_ISSET(socket_for_client, &send_fds))
//                                    {
//                                        status = send(socket_for_client, buffer+sended_bytes, byte_left, 0);        // Worng querry
//                                        if (status == -1UL)
//                                        {
//                                            printf("=>Sending failure !!!");
//                                            if (try_times ++ < 3)
//                                            {
//                                                break;
//                                            };
//                                        }
//                                        else
//                                        {
//                                            sended_bytes += status;
//                                            byte_left -= status;
//                                        };
//                                    }
//                                    else
//                                    {
//                                        printf("=> Socket is not ready to send data!! \n");
//                                        if (try_times ++ < 3)
//                                        {
//                                            printf("=> Error on sending message");
//                                            break;
//                                        };
//                                    };
//                                };
//                                locker_fd_set.unlock();
                            };
                        };
                    };
                }
                else
                {
                    printf("=> Wrong format message or no massge to send !! \n");
                };
            };
            msg_wts.pop();
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
//                std::unique_lock<std::mutex> locker(user_command_muxtex);
                msg_wts.push(message);
//                locker.unlock();
//                cond.notify_all();
            };
        }
        else
        {
            printf("=> Wrong format stype recived message \n");
        };
    };
};
