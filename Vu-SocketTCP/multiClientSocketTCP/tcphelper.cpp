#include "tcphelper.h"
#include "iosocket.h"

// TCPhelper contructor, menthod, .....
//std::vector<TCPhelper::rps_timeout> TCPhelper::rps_timeout_list;
//const int TCPhelper::timeout;

void ultoc(unsigned int& ul, unsigned char* cu)
{
    ul = 0;
    for (unsigned int i=0; i<4; i++)
    {
        unsigned int j = static_cast<unsigned int>(*cu++) << i*8;
        ul |= j;
    };
};

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

bool TCPhelper::unpacked_msg(msg_text& msg_output, std::vector<unsigned char>& buffer)
{
    if(buffer.size() < 9)
    {
        // Shortest msg has 9 bytes in length.
        return false;
    }
    else
    {
        unsigned int len_msg;
        unsigned char len_msg_c[4] = {buffer[0],buffer[1],buffer[2],buffer[3]};
        ultoc(len_msg,len_msg_c);

        if(buffer.size() < len_msg)
        {
            return false;
        }
        else
        {
            msg_output.type_msg = buffer[4];

            unsigned char ID_msg_c[4] = {buffer[5],buffer[6],buffer[7],buffer[8]};
            ultoc(msg_output.ID,ID_msg_c);

            if (msg_output.type_msg < 2)
            {
                msg_output.msg.clear();

                for(unsigned long i=9; i<len_msg; i++)
                {
                    msg_output.msg = msg_output.msg + static_cast<char>(buffer[i]);
                };
            };

            std::vector<unsigned char> buffer_remain(buffer.begin()+len_msg, buffer.end());
            buffer.clear();
            buffer = buffer_remain;

            return true;
        };
    };
};

bool TCPhelper::packed_msg(msg_text& msg_input, std::vector<unsigned char>& element)
{
    // Length msg
    unsigned long len = msg_input.msg.length()+9;
    unsigned char len_byte[4];
    memcpy(len_byte, &len, 4);

    // ID msg
    unsigned char ID_byte[4];
    static unsigned int ID_msg = 1;

    ID_msg = (ID_msg==0)?ID_msg++:ID_msg;

    if (msg_input.type_msg < 3)
    {
        memcpy(ID_byte, &ID_msg, 4);
        msg_input.ID = ID_msg;
    }
    else
    {
        if(msg_input.ID == 0)
        {
            printf("=> Haven't insert ID of the msg you want to send respond");
            return false;
        }
        else
        {
            memcpy(ID_byte, &msg_input.ID, 4);            
        }
    };


    for(unsigned int i = 0; i<len; i++)
    {
        if (i<4)
        {
            element.push_back(len_byte[i]);
        }
        else if (i == 4)
        {
            element.push_back(msg_input.type_msg);
        }
        else if (i<9)
        {
            element.push_back(ID_byte[i-5]);
        }
        else
        {
            if (msg_input.type_msg<2)
            {
                element.push_back(static_cast<unsigned char>(msg_input.msg[i-9]));
            }
        };
    };
    ID_msg++;
    return true;
}

void TCPclient::send_msg(msg_queue& msg_wts, const int client_oder, int socket_fd)
{
    if (user_name[client_oder].empty())
    {
        printf("=> Login local faile!!!. Plz restart programme!");
        exit(EXIT_FAILURE);
    };

    msg_text msg_login;
    msg_login.msg = user_name[client_oder];
    msg_login.type_msg = SGI;

    std::vector<unsigned char> login_packed;
    this -> packed_msg(msg_login, login_packed);
    msg_wts.push(login_packed, Q_MSG);

    std::unique_lock<std::mutex> lock_ping(this->ping_mutex, std::defer_lock);

    while(!end_connection[client_oder])
    {
        int type_msg_send = -1;
        std::vector<unsigned char> element;
        element.clear();

        if (this->ping == false)
        {
            if (!msg_wts.is_empty(Q_RSP))
            {
                type_msg_send = RSP;
                element = msg_wts.get(Q_RSP);
            }
            else if(!msg_wts.is_empty(Q_MSG))
            {
                type_msg_send = MSG;
                element = msg_wts.get(Q_MSG);
            };            
        }
        else
        {
            lock_ping.lock();
            type_msg_send = PIG;
            this -> packed_msg(ping_msg, element);
        };


        if(element.empty())
        {
            // There is no msg or rsp to send
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else
        {
            // Prepared sending buffer
            unsigned char buffer[element.size()];
            std::copy(element.begin(), element.end(), buffer);

            // Send all msg
            fd_set send_fds;
            unsigned long total_bytes, byte_left, sended_bytes;
            total_bytes = element.size();
            byte_left = total_bytes;
            sended_bytes = 0;
            int try_times = 0;

            while(sended_bytes < total_bytes)
            {
                FD_ZERO(&send_fds);
                FD_SET(socket_fd,&send_fds);
                if (select(socket_fd+1,nullptr, &send_fds, nullptr, &general_tv) <0)
                {
                    perror("=> Error with select on this socket");
                    end_connection[client_oder] = true;
                    is_error[client_oder] = true;
                    break;
                };

                if(FD_ISSET(socket_fd, &send_fds))
                {
                    long status = send(socket_fd, buffer+sended_bytes, byte_left, 0);
                    if (status < 0)
                    {
                        // Undefined error on send().
                        if (try_times++ > 3)
                        {
                            printf("=> Sending failure !!!");
                            end_connection[client_oder] = true;
                            is_error[client_oder] = true;
                            break;
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
                        end_connection[client_oder] = true;
                        is_error[client_oder] = true;
                        break;
                    };
                };
            };

            if(RSP == type_msg_send)
            {
                msg_wts.pop(Q_RSP);
            }
            else if ((MSG == type_msg_send) || (SGI == type_msg_send) )
            {
                msg_wts.pop(Q_MSG);

                msg_text msg_unpacked;
                this->unpacked_msg(msg_unpacked, element);

                rps_timeout timepoint;
                timepoint. msg = msg_unpacked;
                timepoint.timeout = std::chrono::system_clock::now();
                timepoint.socket = socket_fd;
                rps_timeout_list.push_back(timepoint);
            }
            else if (PIG == type_msg_send)
            {
                rps_timeout timepoint;
                timepoint. msg = ping_msg;
                timepoint.timeout = std::chrono::system_clock::now();
                timepoint.socket = socket_fd;
                rps_timeout_list.push_back(timepoint);

                lock_ping.unlock();
                con_ping.notify_all();

                // Wait until get the result of ping.
                lock_ping.lock();
                con_ping.wait(lock_ping);
            };
        };
    };
};

void TCPhelper::msg_confirm(const msg_text rsp)
{
    unsigned long i;
    for (i=0; i < this->rps_timeout_list.size(); i++)
    {
        if(rps_timeout_list[i].msg.ID == rsp.ID)
        {
            rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
            break;
        };
    };
}

// TCPclient contructor, nmenthod,....
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
//        printf("=> Created a client's socket decriptor!!\n");
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

    struct timeval general_tv;
    general_tv.tv_sec = 5;
    general_tv.tv_usec = 5000;

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

int TCPclient::recv_msg(const int client_fd, msg_queue& msg_wts, thread_pool& threads, const int client_oder)
{
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(client_fd, &read_fds);
    if (select(client_fd+1,&read_fds, nullptr, nullptr, &general_tv) <0)
    {
        perror("=> Select");
        return -1;
    };

    if (FD_ISSET(client_fd, &read_fds))
    {
        const unsigned int bufsize = 20;
        unsigned char recv_buf[bufsize] = {0};
        long num_data;

        if ((num_data = recv(client_fd,recv_buf,bufsize,0)) <=0 )
        {
            if ((num_data == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
            {
                // Recv() is still happening.
                perror("=> Message is not gotten complete !!");
                return 0;
            }
            else
            {
                if (num_data == 0)
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
            threads.enqueue([=, &msg_wts]()
            {
                this->process_on_buffer_recv(recv_buf, num_data, msg_wts, client_oder);
            });
            return 0;
        };
    }
    else
    {
        return 0;
    };
};

void TCPclient::process_on_buffer_recv(const unsigned char buffer[], const long num_data, msg_queue& msg_wts, const int client_oder)
{
    // Process
    std::unique_lock<std::mutex> lock_buffer(this -> buffer_mutex);
    std::vector<unsigned char> buffer_all;
    buffer_all = this -> buffer;
    buffer_all.insert(buffer_all.end(), &buffer[0], &buffer[num_data]);
    this->buffer.clear();

    while(buffer_all.size() > 0)
    {
        msg_text msg_get;
        bool is_msg_usable = this->unpacked_msg(msg_get, buffer_all);
        if (!is_msg_usable)
        {
            this->buffer = buffer_all;
            break;
        }
        else
        {
            if(RSP == msg_get.type_msg)
            {
                this->msg_confirm(msg_get);
            }
            else
            {
                // Push rsp
                msg_text rsp;
                rsp.type_msg = RSP;
                rsp.ID = msg_get.ID;

                std::vector<unsigned char> element;
                this -> packed_msg(rsp, element);
                msg_wts.push(element, Q_RSP);

                std::unique_lock<std::mutex> lock_buffer(log_mutext);
                printf("Message %d for client %s from server: %s \n", static_cast<int>(msg_get.ID), user_name[client_oder].c_str(),msg_get.msg.c_str());
//                std::cout << "Message "<< static_cast<int>(msg_get.ID)<<" from server :" << msg_get.msg <<std::endl;
            };
        };
    };
    lock_buffer.unlock();
}

void TCPclient::timeout_clocker(const int client_oder)
{
    rps_timeout_list.clear();

    while(!end_connection[client_oder])
    {
        if(!rps_timeout_list.empty())
        {
            if (this->ping == false)
            {
                // If no ping is processed, keep chech timeout for the first msg ID in the list
                std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list.front().timeout;
                if(duration.count() > timeout)
                {
                    this -> ping = true;
                    printf("=> Start new PING \n");
                };
            }
            else
            {
                std::unique_lock<std::mutex> lock_ping(this->ping_mutex, std::defer_lock);
                lock_ping.lock();
                con_ping.wait(lock_ping);
                printf("   Chech PING rsp ..... \n");

                while(!end_connection[client_oder])
                {
                    // If ping is processed, check rps of ping. If timeout, stop and restart socket.
                    bool is_stil_ping = false;
                    unsigned long i;
                    for (i=0; i < rps_timeout_list.size(); i++)
                    {
                        if(rps_timeout_list[i].msg.ID == ping_msg.ID)
                        {
                            std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list[i].timeout;
                            if (duration.count() > timeout)
                            {
                                printf("   PING timeout !!! \n");
                                rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
                                end_connection[client_oder] = true;
                                is_error[client_oder] = true;
                                this -> ping = false;

                                lock_ping.unlock();
                                con_ping.notify_all();
                            };
                            is_stil_ping = true;
                            break;
                        };
                    };

                    // Rps_ping has been deleted by msg_confirm.
                    if(false == is_stil_ping)
                    {
                        this -> ping = false;
                        lock_ping.unlock();
                        con_ping.notify_all();

                        printf("   PING cleared !!! \n");
                    }
                    else
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    };
                };
            };
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        };
    };
}


