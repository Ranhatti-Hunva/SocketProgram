#include "iosocket.h"

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

void read_terminal(bool& end_connection, client_list& client_socket_list, TCPserver& server_helper, msg_queue& msg_wts)
{
    while(!end_connection)
    {
        fd_set reader;
        FD_ZERO(&reader);
        FD_SET(0,&reader);

        int data = select(1,&reader, nullptr, nullptr, &server_helper.general_tv);
        if (data > 0)
        {
            if (FD_ISSET(0, &reader))
            {
                std::string user_cmd_str;
                getline(std::cin, user_cmd_str);
                if(!user_cmd_str.empty())
                {
                    if(user_cmd_str.compare("#"))
                    {
                        // Packet msg and push to msg_send_queue
                        std::vector<std::string> container;
                        splits_string(user_cmd_str, container);

                        if(container.size() < 2)
                        {
                            printf("=> Wrong format message or no massge to send !! \n");
                        }
                        else
                        {
                            // Is part 1 of container is a number.
                            // int client_socket = stoi(container[0]);
                            // Searching client
                            int client_socket = client_socket_list.get_fd_by_user_name(container[0].c_str());
                            client_socket = client_socket_list.is_online(client_socket);

                            if(client_socket < 0)
                            {
                                printf("=> Sorry, %s is not online now!!. \n", container[0].c_str());
                            }
                            else
                            {
                                std::string msg;
                                unsigned long len_msg = user_cmd_str.length() - container[0].length()-1;
                                unsigned long start_pos =  container[0].length()+1;
                                msg = user_cmd_str.substr(start_pos, len_msg);

                                // Force close connection with client by server.
                                if (!msg.compare("#"))
                                {
                                    server_helper.closer(client_socket,client_socket_list);
                                    printf("=> Closed connection with soket %d.\n",client_socket);
                                }
                                else
                                {
                                    // Packed msg and push to msg_queue
                                    msg_text msg_send;
                                    msg_send.msg = "Server >" + msg;
                                    msg_send.type_msg = MSG;

                                    q_element element;
                                    element.socket_fd = client_socket;
                                    server_helper.packed_msg(msg_send, element.content);
                                    msg_wts.push(element, MSG);
                                };
                            };
                        };
                    }
                    else
                    {
                        end_connection = true;
                        break;
                    };
                };
            };
        };
    };
};


