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

void send_TCP(msg_queue& msg_wts, client_list& client_socket_list, TCPserver& server_helper, bool& end_connection)
{
    std::vector<std::string> container;
    int socket_for_client;
    client_information client_socket_information;

    while(!end_connection)
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
            if(!user_cmd_str.empty())
            {
                msg_wts.push_msg(user_cmd_str);
            };
        };

        // Get and send msg or respond
        std::string msg;
        msg.clear();
        bool is_respond = false;

        // Prioritize send all respond before send msg
        if (!msg_wts.respond_empty())
        {
            is_respond = true;
            msg = msg_wts.respond_get();
        }
        else if(!msg_wts.msg_empty())
        {
            is_respond = false;
            msg = msg_wts.msg_get();
        }

        if (!msg.empty())
        {

            if (!msg.compare("#"))
            {
                // Exit thread because of the wish of user.
                end_connection = true;
                break;
            }
            else
            {
                // Send message
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
                        printf("=>The folowing text is not a number !! \n");
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
                }
                else
                {
                    // Get file msg and send to client.
                    std::ifstream read_file;
                    std::string nameof = "../build-serverSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/clientmsg/"+container[1]+".txt";
                    read_file.open(nameof);

                    if (!read_file.fail())
                    {
                        std::string msg;
                        std::stack<std::string> msg_lock_in;

                        int forward_fd = client_socket_list.get_fd_by_user_name(container[1].c_str());
                        forward_fd = client_socket_list.is_online(forward_fd);

                        while(!read_file.eof())
                        {
                            getline(read_file,msg);
                            msg = msg + "/" + std::to_string(forward_fd);
                            msg_lock_in.push(msg);
                        };
                        msg_lock_in.pop();

                        while(!msg_lock_in.empty())
                        {
                            msg_wts.push_msg(msg_lock_in.top());
                            msg_lock_in.pop();
                        };

                        read_file.close();

                        if( remove( nameof.c_str() ) != 0 )
                            perror( "=> Error deleting file" );
                        else
                            puts( "=> File successfully deleted" );
                    }
                    else
                    {
                        printf("=> History msg of this client is clear!! \n");
                        read_file.close();
                    };
                };
            }
            else
            {
                std::string message;
                // Format message
                int forward_fd = client_socket_list.get_fd_by_user_name(container[1].c_str());
                forward_fd = client_socket_list.is_online(forward_fd);

                if (forward_fd < 0)
                {
                    message = "Sorry,"+container[1]+" can't rely you right now!!"+"/"+std::to_string(client_fd);
                    // Save msg with file name is the name of forward user.
                    std::ofstream write_file;
                    std::string nameof = "../build-serverSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/clientmsg/"+container[1]+".txt";
                    write_file.open(nameof, std::fstream::app);

                    client_information host_msg;
                    client_socket_list.get_by_fd(client_fd, host_msg);
                    std::string str(host_msg.user_name);

                    write_file <<  str+"=>"+container[0]  << std::endl;

                    write_file.close();
                }
                else
                {
                    client_information host_msg;
                    client_socket_list.get_by_fd(client_fd, host_msg);

                    std::string str(host_msg.user_name);
                    message = str+"=>"+container[0]+"/"+std::to_string(forward_fd);
                }
                // Push msg to msg waiting queue.
                msg_wts.push_msg(message);
            };
        }
        else
        {
            printf("=> Wrong format stype recived message \n");
        };
    };
};
