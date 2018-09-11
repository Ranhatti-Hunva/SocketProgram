#include "tcphelper.h"
#include "iosocket.h"

static std::mutex log_mutex;

// TCPhelper contructor, menthod, .....
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

    if (msg_input.type_msg != RSP)
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

void TCPhelper::msg_confirm(const msg_text rsp)
{
    unsigned long i;
    for (i=0; i < this->rps_timeout_list.size(); i++)
    {
        if(rps_timeout_list[i].msg.ID == rsp.ID)
        {
//            std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list[i].timeout;
//            printf("=> Get RSP for msg %d, respondre time: %f \n", rsp.ID, duration.count());

            rps_timeout_list.erase(rps_timeout_list.begin()+static_cast<long>(i));
            break;
        };
    };
}

// TCPserver contructor, menthod, .....
int TCPserver::server_echo(int port_num)
{
    int server_fd = -1;
    struct addrinfo *IP_list, *p;

//    IP_list = this->get_addinfo_list("10.42.0.187",port_num);
    IP_list = this->get_addinfo_list("",port_num);
    for(p = IP_list; p != nullptr; p = p->ai_next)
    {
        if ((server_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1)
        {
            continue;
        }

        if (::bind(server_fd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(server_fd);
            server_fd = -1;
            continue;
        };
        break;
    };

    if (p == nullptr)
    {
        fprintf(stderr, "=> Server: failed to bind\n");
        exit(EXIT_FAILURE);
    };

    // Show the IP infor to terminal
    struct sockaddr_in *addr_used = reinterpret_cast<struct sockaddr_in *>(p->ai_addr);
    void *addr_infor = &(addr_used -> sin_addr);
    char ipstr[INET_ADDRSTRLEN];
    inet_ntop(p->ai_family, addr_infor, ipstr, sizeof(ipstr));

    printf("=> Create server socket succesfully.\n");
    printf("=> Server's using the IPv4: %s - %d. \n", ipstr, port_num);

    if (listen(server_fd, 10) == -1)
    {
        perror("=> Listen");
        exit(EXIT_FAILURE);
    };

    // Prepare data for select() later.
    FD_SET(server_fd,&(this->master));
    this->fd_max = server_fd;

    // Set non-blocking to server.
    long arg = fcntl(server_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(server_fd, F_SETFL, arg);

    printf("=> Server's listening!! \n");
    printf("=> Enter # to close server socket \n\n");
    freeaddrinfo(IP_list);

    return server_fd;
};

int TCPserver::acceptor(int server_fd, client_list& client_socket_list)
{
    struct sockaddr client_addr;
    unsigned int size_client_address = sizeof(client_addr);
    int client_socket;

    if((client_socket = accept(server_fd, &client_addr, &size_client_address))<0)
    {
        return -1;
    }
    else
    {
        std::unique_lock<std::mutex> locker(fd_set_mutex);
        FD_SET(client_socket, &master);
        client_fds.push_back(client_socket);
        fd_max = (client_socket > fd_max)?client_socket:fd_max;
        locker.unlock();

        client_socket_list.add_fd(client_socket);

        // Set non-blocking to new socket.
        long arg = fcntl(client_socket, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(server_fd, F_SETFL, arg);

        // Show client information to terminal
        char IPclient[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.sa_family,&(reinterpret_cast<struct sockaddr_in* >(&client_addr)->sin_addr),IPclient, INET6_ADDRSTRLEN);

//        {
//            std::unique_lock<std::mutex> log_log(log_mutex);
//            printf("=> New conection from %s on socket %d\n",IPclient,client_socket);
//        };
    };
    return client_socket;
};

int TCPserver::recv_msg(int server_fd, client_list& client_socket_list, msg_queue& msg_wts, thread_pool& threads)
{
    std::unique_lock<std::mutex> locker(fd_set_mutex);
    fd_set read_fds = this->master;
    if (select(this->fd_max+1,&read_fds, nullptr, nullptr, &general_tv) <0)
    {
        perror("=> Select");
        exit(EXIT_FAILURE);
    };
    locker.unlock();

    if (FD_ISSET(server_fd, &read_fds))
    {
        // Open new thread to accept the new connection;
        threads.enqueue([&]()
        {
            this->acceptor(server_fd, client_socket_list);
        });
    };

    for (unsigned long i=0; i < this->client_fds.size(); i++)
    {
        if(FD_ISSET(this->client_fds[i], &read_fds))
        {
            const unsigned int bufsize = 20;
            unsigned char recv_buffer[bufsize] = {0};
            long num_data = 0;

            if ((num_data=recv(this->client_fds[i], recv_buffer, bufsize,0)) <=0)
            {
                if ((num_data == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                {
                    perror("=> Message is not gotten complete !!");
                }
                else
                {
                    if (num_data == 0)
                    {
                        printf("=> Connect client has been close, soket: %d\n",this->client_fds[i]);
                    }
                    else
                    {
                        printf("=> Error socket.\n");
                    };
                    this->closer(this->client_fds[i], client_socket_list);
                };
            }
            else
            {
                q_element element;
                element.content.insert(element.content.end(), &recv_buffer[0], &recv_buffer[num_data]);
                element.socket_fd = this->client_fds[i];
                msg_wts.push(element, Q_RECV);
            };
        };
    };
    return 0;
};

void TCPserver::process_on_buffer_recv(const msg_text msg_get, client_list& client_socket_list, const client_information* host_msg, msg_queue& msg_wts)
{
    // Process
    if(RSP == msg_get.type_msg)
    {
        this -> msg_confirm(msg_get);
    }
    else
    {
        // Push rsp
        msg_text rsp;
        rsp.type_msg = RSP;
        rsp.ID = msg_get.ID;

        q_element element;
        element.socket_fd = host_msg->socket_fd;
        this -> packed_msg(rsp, element.content);
        msg_wts.push(element, Q_RSP);

        // Analyser msg got
        switch(msg_get.type_msg)
        {
        case PIG:
        {
            std::unique_lock<std::mutex> log_log(log_mutex);
            std::cout << "=> Message PING - "<< static_cast<int>(msg_get.ID)<<" from client on socket: " << host_msg->socket_fd << std::endl;
            break;
        };
        case SGI:
        {
            {
                std::unique_lock<std::mutex> log_log(log_mutex);
                std::cout << "=> Message LOGIN - "<< static_cast<int>(msg_get.ID)<<" from client on socket " << host_msg->socket_fd << ":" << msg_get.msg <<std::endl;
            };

            int old_socket_fd = client_socket_list.set_user_name(host_msg->socket_fd,msg_get.msg.c_str());
            if((old_socket_fd > 0) && (old_socket_fd != host_msg->socket_fd))
            {
                std::unique_lock<std::mutex> locker(fd_set_mutex);
                FD_CLR(old_socket_fd, &master);
                close(old_socket_fd);
                for (unsigned long i=0; i < this->client_fds.size(); i++)
                {
                    if (client_fds[i] == old_socket_fd)
                    {
                        client_fds.erase(client_fds.begin()+static_cast<long>(i));
                    };
                };
                locker.unlock();
            }
            else
            {
                // Get file msg and send to client.
                std::ifstream read_file;
                std::string nameof = "../build-serverSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/clientmsg/"+msg_get.msg+".txt";
                read_file.open(nameof);

                if (!read_file.fail())
                {
                    msg_text msg_history;
                    msg_history.type_msg = MSG;

                    while(!read_file.eof())
                    {
                        getline(read_file,msg_history.msg);
                        if(!msg_history.msg.empty())
                        {
                            q_element element;
                            element.socket_fd = host_msg->socket_fd;
                            this -> packed_msg(msg_history, element.content);
                            msg_wts.push(element, Q_MSG);
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                        };
                    };
                    read_file.close();

                    if( remove( nameof.c_str() ) != 0 )
                        perror( "=> Error deleting file \n" );
//                    else
//                        printf( "=> File history successfully deleted \n" );
                }
                else
                {
//                    printf("=> History msg of this client is clear!! \n");
                    read_file.close();
                };
            };
            break;
        };
        case MSG:
        {
            {
                std::unique_lock<std::mutex> log_log(log_mutex);
                printf("=> Message %d from client on socket %d : %s \n", static_cast<int>(msg_get.ID), host_msg->socket_fd, msg_get.msg.c_str());
            };

            std::vector<std::string> container;
            splits_string(msg_get.msg, container);

            msg_text msg_trasnsfer;
            msg_trasnsfer.type_msg = MSG;

            if(container[0].compare("all"))
            {
                // MSG: forward_user_name/msg;
                // Format message
                int forward_fd = client_socket_list.get_fd_by_user_name(container[0].c_str());
                forward_fd = client_socket_list.is_online(forward_fd);

                if (forward_fd < 0)
                {
                    msg_trasnsfer.msg  = "Sorry,"+container[0]+" can't rely you right now!!";

                    q_element element;
                    element.socket_fd = host_msg->socket_fd;
                    this -> packed_msg(msg_trasnsfer, element.content);
                    msg_wts.push(element, Q_MSG);

                    // Save msg with file name is the name of forward user.
                    std::ofstream write_file;
                    std::string nameof = "../build-serverSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/clientmsg/"+container[0]+".txt";
                    write_file.open(nameof, std::fstream::app);

                    std::string str(host_msg->user_name);

                    write_file << str+">"+container[1]<< std::endl;

                    write_file.close();
                }
                else
                {
                    std::string str(host_msg->user_name);
                    msg_trasnsfer.msg = str+">"+container[1];

                    q_element element;
                    element.socket_fd = forward_fd;
                    this -> packed_msg(msg_trasnsfer, element.content);
                    msg_wts.push(element, Q_MSG);
                };
            }
            else
            {
                std::string str(host_msg->user_name);
                msg_trasnsfer.msg = str+">"+container[1];

                for (unsigned long i=0; i< client_socket_list.size(); i++)
                {
                    std::string message;
                    client_information* forward_msg;
                    forward_msg = client_socket_list.get_by_order(i);

                    if(forward_msg->socket_fd != host_msg->socket_fd)
                    {
                        q_element element;
                        element.socket_fd =  forward_msg->socket_fd;
                        this -> packed_msg(msg_trasnsfer, element.content);
                        msg_wts.push(element, Q_MSG);
                    };
                };
            };
            break;
        };
        };
    };
};

void TCPserver::send_msg(msg_queue& msg_wts, bool& end_connection, client_list& client_socket_list)
{
    while(!end_connection)
    {
        // Prioritize send all respond before send msg
        bool is_rsp = false;

        q_element element;
        element.content.clear();

        if (!msg_wts.is_empty(Q_RSP))
        {
            is_rsp = true;
            element = msg_wts.get(Q_RSP);
        }
        else if(!msg_wts.is_empty(Q_MSG))
        {
            is_rsp = false;
            element = msg_wts.get(Q_MSG);
        };

        if(element.content.empty())
        {
            // There is no msg or rsp to send
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        else
        {
            // Prepared sending buffer
            unsigned char buffer[element.content.size()];
            std::copy(element.content.begin(), element.content.end(), buffer);

            // Send all msg
            fd_set send_fds;
            unsigned long total_bytes, byte_left, sended_bytes;
            total_bytes = element.content.size();
            byte_left = total_bytes;
            sended_bytes = 0;
            int try_times = 0;

            while(sended_bytes < total_bytes)
            {
                FD_ZERO(&send_fds);
                FD_SET(element.socket_fd,&send_fds);
                if (select(element.socket_fd+1,nullptr, &send_fds, nullptr, &general_tv) <0)
                {
                    perror("=> Error with select on this socket");
                    this->closer(element.socket_fd,client_socket_list);
                    break;
                };

                if(FD_ISSET(element.socket_fd, &send_fds))
                {
                    long status = send(element.socket_fd, buffer+sended_bytes, byte_left, 0);
                    if (status < 0)
                    {
                        // Undefined error on send().
                        if (try_times++ > 3)
                        {
                            printf("=> Sending failure !!!");
                            this->closer(element.socket_fd,client_socket_list);
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
                        this->closer(element.socket_fd,client_socket_list);
                    };
                };
            };

            if(is_rsp)
            {
                msg_wts.pop(Q_RSP);
            }
            else
            {
                msg_wts.pop(Q_MSG);

                msg_text msg_unpacked;
                this->unpacked_msg(msg_unpacked, element.content);

                rps_timeout timepoint;
                timepoint. msg = msg_unpacked;
                timepoint.timeout = std::chrono::system_clock::now();
                timepoint.socket = element.socket_fd;
                rps_timeout_list.push_back(timepoint);

//                cout<< "=> Send msg "<< msg_unpacked.ID << " on socket " << element.socket_fd <<" - content:" << msg_unpacked.msg << endl;
            }
        };
    };
};

void TCPserver::post_send_process(q_element element, const bool is_rsp, msg_queue& msg_wts)
{
    if(is_rsp)
    {
        msg_wts.pop(Q_RSP);
    }
    else
    {
        msg_wts.pop(Q_MSG);

        msg_text msg_unpacked;
        this->unpacked_msg(msg_unpacked, element.content);

        rps_timeout timepoint;
        timepoint. msg = msg_unpacked;
        timepoint.timeout = std::chrono::system_clock::now();
        timepoint.socket = element.socket_fd;
        rps_timeout_list.push_back(timepoint);
    }
}

void TCPserver::closer(int client_fd, client_list& client_socket_list)
{
    client_socket_list.off_client(client_fd);

    std::unique_lock<std::mutex> locker(fd_set_mutex);
    FD_CLR(client_fd, &master);
    close(client_fd);
    for (unsigned long i=0; i < this->client_fds.size(); i++)
    {
        if (client_fds[i] == client_fd)
        {
            client_fds.erase(client_fds.begin()+static_cast<long>(i));
        };
    };
    locker.unlock();
}

void TCPserver::timeout_clocker(bool& end_connection, client_list& client_socket_list)
{
    while(!end_connection)
    {
        if(!rps_timeout_list.empty())
        {
            std::chrono::duration<float> duration = std::chrono::system_clock::now() - rps_timeout_list.front().timeout;
            if(duration.count() > timeout)
            {
                client_socket_list.off_client(rps_timeout_list.front().socket);
                rps_timeout_list.erase(rps_timeout_list.begin());
            };
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        };
    };
}

void TCPserver::buffer_analyser(bool& end_connection, msg_queue& msg_wts, client_list& client_socket_list, thread_pool& threads)
{
    while(!end_connection)
    {
        if(!msg_wts.is_empty(Q_RECV))
        {
            q_element element = msg_wts.get(Q_RECV);

            if(client_socket_list.is_online(element.socket_fd) > 0)
            {
                client_information* host_msg;
                host_msg = client_socket_list.get_by_fd(element.socket_fd);

                std::vector<unsigned char> buffer_all;
                buffer_all = host_msg->buffer;
                buffer_all.insert(buffer_all.end(), element.content.begin(), element.content.end());

                host_msg->buffer.clear();

                while(buffer_all.size()>0)
                {
                    msg_text msg_get;
                    bool is_msg_usable = this->unpacked_msg(msg_get, buffer_all);
                    if(!is_msg_usable)
                    {
                        host_msg->buffer = buffer_all;
                        break;
                    }
                    else
                    {
                        threads.enqueue([=, &client_socket_list, &msg_wts]()
                        {
                            this -> process_on_buffer_recv(msg_get, client_socket_list, host_msg, msg_wts);
                        });
                    };
                };
            };

            msg_wts.pop(Q_RECV);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}


