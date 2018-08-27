#include "tcphelper.h"


// TCPhelper contructor, menthod, .....
std::vector<TCPhelper::rps_timeout> TCPhelper::rps_timeout_list;
const int TCPhelper::timeout;

TCPhelper::TCPhelper()
{
    FD_ZERO(&master);
    fd_max = 0;

    general_tv.tv_sec = 0;
    general_tv.tv_usec = 5000;
}

struct addrinfo* TCPhelper::get_addinfo_list(std::string host_name, int port_num)
{
    struct addrinfo* infor_list;        // servinfor is linked-list which contain all address information.
    long status;                        // Check error from getaddrinfor().Status return no-zero if there's an error.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.

    if(host_name.empty())
    {
        hints.ai_flags = AI_PASSIVE;    // AI_PASSIVE: assigned the address of local host tho the socket structure.
        status = getaddrinfo(nullptr, std::to_string(port_num).c_str(), &hints, &infor_list);
    }
    else
    {
        status = getaddrinfo(host_name.c_str(), std::to_string(port_num).c_str(), &hints, &infor_list);
    };

    if (status!=0)
    {
        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(static_cast<int>(status)));
        fprintf(stderr, "%d", static_cast<int>(status));
        exit(EXIT_FAILURE);
    };
    return infor_list;
};

bool TCPhelper::unpacked_msg(char* buffer, std::string& msg_incomplete, char& ID_msg_incomplete)
{
    char* p = buffer;
    while(*p != 0)
    {
        switch (*p)
        {
        case 2:
        {
            msg_incomplete.clear();
            p++;
            ID_msg_incomplete = *p;
            break;
        };
        case 3:
        {
            return true;
        };
        default:
        {
            msg_incomplete = msg_incomplete + *p;
            break;
        };
        };
        p++;
    };
    return false;
}

char TCPhelper::packed_msg(std::string& msg)
{
    static char ID = 1;
    while ((ID==0)||(ID==2)||(ID==3))
    {
        ID++;
    };

    char begin_c = 2;
    char end_c = 3;

    msg = ID + msg + end_c;
    msg = begin_c + msg;

    return ID++;
}

bool TCPhelper::send_msg(int fd, std::string msg, std::vector<rps_timeout>& rps_queue_timeout, bool& is_rps)
{
    // Packed and attack the ID for msg. ID is a char.
    char ID_message = this->packed_msg(msg);

    const unsigned long length_msg = msg.length()+1;
    char send_buffer[length_msg];
    memset(&send_buffer, 0, length_msg);

    strcpy(send_buffer, msg.c_str());

    // Put all message to buffer.
    fd_set send_fds;

    unsigned long total_bytes, byte_left, sended_bytes;
    total_bytes = msg.length();
    byte_left = total_bytes;
    sended_bytes = 0;
    int try_times = 0;

    while(sended_bytes < total_bytes)
    {
        FD_ZERO(&send_fds);
        FD_SET(fd,&send_fds);
        if (select(fd+1,nullptr, &send_fds, nullptr, &general_tv) <0)
        {
            perror("=>Select ");
            return false;
        };

        if(FD_ISSET(fd, &send_fds))
        {
            long status = send(fd, send_buffer+sended_bytes, byte_left, 0);
            if (status < 0)
            {
                // Undefined error on send().
                if (try_times++ > 3)
                {
                    printf("=> Sending failure !!!");
                    return false;
                };
            }
            else
            {
                // Send a part of message.
                sended_bytes += static_cast<unsigned long>(status);
                byte_left -= static_cast<unsigned long>(status);
            };
        }
        else
        {
            // Timeout when socket buffer is full.
            printf("=> Socket is not ready to send data!! \n");
            if (try_times++ > 3)
            {
                printf("=> Error on sending message");
                return false;
            };
        };
    };

    // Save ID massge to waiting respond.
    if (!is_rps)
    {
        rps_timeout timepoint;
        timepoint.ID = ID_message;
        timepoint.timeout = std::chrono::system_clock::now();
        timepoint.socket = fd;
        rps_queue_timeout.push_back(timepoint);
    };
    return true;
};

void TCPhelper::msg_confirm(const std::string rps)
{
    const char ID_msg = rps[3];
    unsigned long i;
    for (i=0; i < this->rps_timeout_list.size(); i++)
    {
        if(rps_timeout_list[i].ID == ID_msg)
        {
            rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
            break;
        };
    };
}

// TCPclient contructor, nmenthod,....
bool TCPclient::ping;
char TCPclient::ping_msg_ID;

int TCPclient::connect_with_timeout(struct addrinfo *server_infor)
{
    // Creat client socket
    int client_fd;
    if ((client_fd = socket(server_infor->ai_family, server_infor->ai_socktype, server_infor->ai_protocol)) < 0)
    {
        perror("=> Socket failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("=> Created a client's socket decriptor!!\n");
    };

    // Change socket to unlocking state.
    long arg = fcntl(client_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, arg);

    // Start to connect with server.
    // If success return client_fd > 0. If fail return -1.
    // Create fd_set for select().
    fd_set connect_timeout;
    FD_ZERO(&connect_timeout);
    FD_SET(client_fd, &connect_timeout);

    long status = connect(client_fd, server_infor->ai_addr,  server_infor->ai_addrlen);
    if (status < 0)
    {
        if (errno == EINPROGRESS)
        {
            // Wait for the connect process.
            if(select(client_fd+1, nullptr, &connect_timeout, nullptr, &general_tv) > 0)
            {
                // Check again error in socket before using.
                int error_check;
                socklen_t lon = sizeof(error_check);
                getsockopt(client_fd, SOL_SOCKET, SO_ERROR, (void*)(&error_check), &lon);
                if (error_check)
                {
                    // Error in socket found. Elimited the socket.
                    fprintf(stderr, "=> Error in connection() %d - %s\n", error_check, strerror(error_check));
                    return -1;
                }
                else
                {
                    // Connect successfull in timetout;
                    this->fd_max = client_fd;
                    FD_SET(client_fd,&(this->master));
                    return client_fd;
                }
            }
            else
            {
                // Connection timeout.
                printf("=> Connection timeout \n");
                return -1;
            };
        }
        else
        {
            // Undetermined error soket.
            printf("=> Error in connection()\n");
            return -1;
        };
    }
    else
    {
        // Connect successfull without wating time.
        this->fd_max = client_fd;
        FD_SET(client_fd,&(this->master));
        return client_fd;
    };
};

int TCPclient::recv_msg(int client_fd)
{
    fd_set read_fds;
    FD_ZERO(&read_fds);

    read_fds = master;
    if (select(fd_max+1,&read_fds, nullptr, nullptr, &general_tv) <0)
    {
        perror("=> Select");
        exit(EXIT_FAILURE);
    };

    long num_byte;

    if (FD_ISSET(client_fd, &read_fds))
    {
        char recv_buf[bufsize] = {0};
        if ((num_byte = recv(client_fd,recv_buf,bufsize,0)) <=0 )
        {
            if ((num_byte == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
            {
                // Recv() is still happening.
                perror("=> Message is not gotten complete !!. See you next time");
                return 0;
            }
            else
            {
                if (num_byte == 0)
                {
                    // Server force close socket
                    printf("=> Connection has been close\n");
                }
                else
                {
                    // Undifined error socket
                    printf("=> Error socket.");
                };
                return -1;
            };
        }
        else
        {
            bool is_msg_usable = this->unpacked_msg(recv_buf, msg_incomplete, ID_msg_incomplete);
//            std::cout << "=> MSG_incomplete:" << msg_incomplete << ". ID msg: "<< (int)ID_msg_incomplete << std::endl;

            return (is_msg_usable)?1:0;
        };
    }
    else
    {
        return 0;
    };
};

void TCPclient::timeout_clocker(bool& end_connection)
{
    while(!end_connection)
    {
        if(!rps_timeout_list.empty())
        {
            if (false == ping)
            {
                // If no ping is processed, keep chech timeout for the first msg ID in the list
                std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list.front().timeout;
                if(duration.count() > timeout)
                {
                    ping = true;
                };
            }
            else
            {
                // If ping is processed, check rps of ping. If timeout, stop and restart socket.
                bool flag_ping = false;
                unsigned long i;
                for (i=0; i < rps_timeout_list.size(); i++)
                {
                    if(rps_timeout_list[i].ID == ping_msg_ID)
                    {
                        std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list[i].timeout;
                        if (duration.count() > timeout)
                        {
                            rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
                            end_connection = true;
                            flag_ping = true;
                        };
                        break;
                    };
                };

                // Rps_ping has been deleted by msg_confirm.
                if(false == flag_ping)
                {
                    rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
                    ping = false;
                };
            }
        }
        else
        {
            // Should to use condition_variable().
            std::this_thread::sleep_for(std::chrono::milliseconds(timeout));
        };
    };
}


