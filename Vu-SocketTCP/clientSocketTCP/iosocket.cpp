#include "iosocket.h"
#include "tcphelper.h"
//#include "udphelper.h"

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

void read_terminal(bool& end_connection, TCPclient& client_helper, msg_queue& msg_wts)
{
    while(!end_connection)
    {
        fd_set reader;
        FD_ZERO(&reader);
        FD_SET(0,&reader);

        struct timeval general_tv;

        general_tv.tv_sec = 0;
        general_tv.tv_usec = 5000;

        int data = select(1,&reader, nullptr, nullptr, &general_tv);
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
                            msg_text msg_send;
                            msg_send.msg = user_cmd_str;
                            msg_send.type_msg = MSG;

                            std::vector<unsigned char> element;
                            client_helper.packed_msg(msg_send, element);
                            msg_wts.push(element, Q_MSG);
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

bool is_reconnect(int& client_fd)
{
    // Ask user if they want to reconnect with server after lose connection.
    std::string answer;
    while (1)
    {
        close(client_fd);
        printf("=> Do you want reconnect to server (Y/N): ");
        getline(std::cin, answer);

        if(!answer.compare("Y"))
        {
            return false;
        }
        else if (!answer.compare("N"))
        {
            return true;
        }
        else
        {
            printf("\n=> Don't understand the answer.\n");
            continue;
        }
    };
};









