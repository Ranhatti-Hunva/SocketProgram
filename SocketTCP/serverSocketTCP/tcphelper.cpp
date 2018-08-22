#include "tcphelper.h"
#include "iosocket.h"


// TCPhelper contructor, menthod, .....
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

bool TCPhelper::unpacked_msg(char* buffer, std::string& msg_incomplete, char& ID_msg_incomplete){
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

char TCPhelper::packed_msg(std::string& msg){
    static char ID = 1;
    while ((ID==0)||(ID==2)||(ID==3))
    {
        ID++;
    };

    char begin_c = 2;
    char end_c = 3;

    msg = ID + msg + end_c;
    msg = begin_c +  msg;

    return ID++;
}

bool TCPhelper::send_msg(int fd, std::string msg, bool& is_rps)
{
    char ID_message = this->packed_msg(msg);

    char send_buffer[bufsize];
    memset(&send_buffer, 0, bufsize);

    // What happend if msg is longer than send_buffer.
    strcpy(send_buffer, msg.c_str());

    fd_set send_fds;
    FD_ZERO(&send_fds);

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
                printf("=> Sending failure !!!");
                if (try_times++ < 3)
                {
                    return false;
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
                return false;
            };
        };
    };
    // Return ID to queue.
    if (!is_rps){
        rps_timeout timepoint;
        timepoint.ID = ID_message;
        timepoint.timeout = std::chrono::system_clock::now();
        timepoint.socket = fd;
        rps_timeout_list.push_back(timepoint);
    };

    return true;
};

std::vector<TCPhelper::rps_timeout> TCPhelper::rps_timeout_list;

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

// TCPserver contructor, menthod, .....
int TCPserver::server_echo(int port_num)
{
    int server_fd = -1;
    struct addrinfo *IP_list, *p;

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
    int socket_for_client;

    if((socket_for_client = accept(server_fd, &client_addr, &size_client_address))<0)
    {
        perror("=> Accept error");
        return -1;
    }
    else
    {
        std::unique_lock<std::mutex> locker(fd_set_mutex);
        FD_SET(socket_for_client, &master);
        client_fds.push_back(socket_for_client);
        fd_max = (socket_for_client > fd_max)?socket_for_client:fd_max;
        locker.unlock();

        client_socket_list.add_fd(socket_for_client);

        // Set non-blocking to new socket.
        long arg = fcntl(socket_for_client, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(server_fd, F_SETFL, arg);

        // Show client information to terminal
        char IPclient[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.sa_family,&(reinterpret_cast<struct sockaddr_in* >(&client_addr)->sin_addr),IPclient, INET6_ADDRSTRLEN);
        printf("=> New conection from %s on socket %d\n",IPclient,socket_for_client);
    };
    return socket_for_client;
};

int TCPserver::reciver(int server_fd, client_list& client_socket_list, msg_queue& msg_wts)
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
        this->acceptor(server_fd, client_socket_list);
    };

    for (unsigned long i=0; i < this->client_fds.size(); i++)
    {
        if(FD_ISSET(this->client_fds[i], &read_fds))
        {
            char recv_buffer[bufsize];
            memset(&recv_buffer,0,bufsize/sizeof(char));
            long status;

            if ((status=recv(client_fds[i], recv_buffer, bufsize,0)) <=0)
            {
                if ((status == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                {
                    perror("=> Message is not gotten complete !!");
                }
                else
                {
                    if (status == 0)
                    {
                        printf("=> Connect client has been close, soket: %d\n",client_fds[i]);
                    }
                    else
                    {
                        printf("=> Error socket.");
                    };

                    this->closer(client_fds[i], client_socket_list);
                };
            }
            else
            {
                client_information host_msg;
                client_socket_list.get_by_fd(client_fds[i],host_msg);

                bool is_msg_usable = this->unpacked_msg(recv_buffer, host_msg.msg_incompleted, host_msg.ID_msg_incompleted);
                if (is_msg_usable)
                {
                    std::cout << "Message "<<(int) host_msg.ID_msg_incompleted <<" from client on socket " << client_fds[i] << ":" << host_msg.msg_incompleted <<std::endl;
                    if (host_msg.msg_incompleted.substr(0,3).compare("RSP"))
                    {
                        std::string RSP = "RSP";
                        std::string msg = RSP + host_msg.ID_msg_incompleted + "/";
                        msg_wts.push_respond(msg+std::to_string(client_fds[i]));

                        if(host_msg.msg_incompleted.compare("PING"))
                        {
                            process_on_buffer_recv(host_msg.msg_incompleted.c_str(), client_socket_list, client_fds[i], msg_wts);
                        };
                    }
                    else
                    {
                        // Delete key message timeout.
                        this->msg_confirm(host_msg.msg_incompleted);
                    };
                };
            };
        };
    };
    return 0;
};

void TCPserver::closer(int server_fd, client_list& client_socket_list)
{
    std::unique_lock<std::mutex> locker(fd_set_mutex);
    FD_CLR(server_fd, &master);
    close(server_fd);
    for (unsigned long i=0; i < this->client_fds.size(); i++)
    {
        if (client_fds[i] == server_fd)
        {
            client_fds.erase(client_fds.begin()+static_cast<long>(i));
        };
    };
    locker.unlock();

    client_socket_list.delete_fs_num(server_fd);
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
