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

void read_terminal(bool& end_connection, client_list& client_socket_list, TCPserver& server_helper, msg_queue& msg_wts)
{
    while(!end_connection)
    {
        fd_set reader;
        FD_ZERO(&reader);
        FD_SET(0,&reader);

        select(1,&reader, nullptr, nullptr, &server_helper.general_tv);
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

                    if(container.size() != 2)
                    {
                        printf("=> Wrong format message or no massge to send !! \n");
                    }
                    else
                    {
                        // Is part 1 of container is a number.
                        try
                        {
                            int client_socket = stoi(container[0]);

                            // Searching client;
                            int is_online = client_socket_list.is_online(client_socket);
                            if(is_online < 0)
                            {
                                printf("=> Sorry, there is no online client on socket: %d \n", client_socket);
                            }
                            else
                            {
                                // Force close connection with client by server.
                                if (!container[1].compare("#"))
                                {
                                    server_helper.closer(client_socket,client_socket_list);
                                    printf("=> Closed connection with soket %d.\n",client_socket);
                                }
                                else
                                {
                                    // Packed msg and push to msg_queue
                                    msg_text msg_send;
                                    msg_send.msg = container[1];
                                    msg_send.type_msg = MSG;

                                    q_element element;
                                    server_helper.packed_msg(msg_send, client_socket, element);
                                    msg_wts.push(element, MSG);

//                                    bool is_packed = server_helper.packed_msg(msg_send, client_socket, element);
//                                    if (is_packed)
//                                    {
//                                        cout << "=> Buffer: " <<endl;
//                                        for(unsigned long i=0; i < element.content.size(); i++)
//                                        {
//                                            cout <<  (int)element.content[i] << " ";
//                                        };
//                                        cout <<  endl;
//                                        msg_wts.push(element, MSG);
//                                    };
                                };
                            };
                        }
                        catch(...)
                        {
                            printf("=> Wrong format message or no massge to send !! \n");
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


