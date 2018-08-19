#include "tcphelper.h"

// TCPhelper contructor, menthod, .....
TCPhelper::TCPhelper(){
    FD_ZERO(&master);
    fd_max = 0;

    general_tv.tv_sec = 1;
    general_tv.tv_usec = 0;
}

struct addrinfo* TCPhelper::get_addinfo_list(std::string host_name, int port_num){
    struct addrinfo* infor_list;        // servinfor is linked-list which contain all address information.
    long status;                        // Check error from getaddrinfor().Status return no-zero if there's an error.

    struct addrinfo hints;              // Clue to find the address information.
    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.

    if(host_name.empty()){
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

// TCPclient contructor, nmenthod,....
int TCPclient::connect_with_timeout(struct addrinfo *server_infor){
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
                // Connection timeput after 3s.
                printf("=> Connection timeout \n");
                return -1;
            };
        }
        else
        {
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

int TCPclient::recv_msg(int client_fd){
    fd_set read_fds;
    FD_ZERO(&read_fds);

    read_fds = master;
    if (select(fd_max+1,&read_fds, nullptr, nullptr, &general_tv) <0)
    {
        perror("=> Select");
        exit(EXIT_FAILURE);
    };

    long num_byte;
    char recv_buf[bufsize];
    memset(&recv_buf,0,bufsize);
    if (FD_ISSET(client_fd, &read_fds))
    {
        memset(&recv_buf,0,bufsize);
        if ((num_byte = recv(client_fd,recv_buf,bufsize,0)) <=0 )
        {
            if ((num_byte == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
            {
                perror("=> Message is not gotten complete !!. See you next time");
                return 0;
            }
            else
            {
                if (num_byte == 0)
                {
                    printf("=> Connection has been close\n");
                }
                else
                {
                    printf("=> Error socket.");
                };
                return -1;
            };
        }
        else
        {
            bool is_valid = unpacked_msg(recv_buf, msg_incomplete);
            return (is_valid)?1:0;
        };
    }
    else
    {
        return 0;
    };
};


// TCPserver contructor, menthod,....
/*
int TCPhelper::server_echo(int port_num){
    int server_fd = -1;
    struct addrinfo *IP_list, *p;

    IP_list = this->get_addinfo_list("",port_num);
    for(p = IP_list; p != nullptr; p = p->ai_next){
        if ((server_fd = socket(p->ai_family, p->ai_socktype,p->ai_protocol)) == -1) {
            continue;
        }

        if (::bind(server_fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(server_fd);
            server_fd = -1;
            continue;
        };
        break;
    };

    if (p == nullptr)  {
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

    if (listen(server_fd, 10) == -1) {
        perror("=> Listen");
        exit(EXIT_FAILURE);
    };
    printf("=> Server's listening!! \n");
    printf("=> Enter # to close server socket \n\n");
    freeaddrinfo(IP_list);

    return server_fd;
};
*/
/*
int TCPhelper::acceptor(int server_fd, std::vector<int>& input_fds, fd_set& master,int& fdmax, client_list& client_socket_list){
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
        FD_SET(socket_for_client, &master);
        input_fds.push_back(socket_for_client);
        client_socket_list.add_fd(socket_for_client);

        fdmax = (socket_for_client > fdmax)?socket_for_client:fdmax;

        fcntl(socket_for_client, F_SETFL, O_NONBLOCK); // Put new socket into non-locking state

        char IPclient[INET6_ADDRSTRLEN];
        inet_ntop(client_addr.sa_family,&(reinterpret_cast<struct sockaddr_in* >(&client_addr)->sin_addr),IPclient, INET6_ADDRSTRLEN);
        printf("=> New conection from %s on socket %d\n",IPclient,socket_for_client);
    };
    return socket_for_client;
};
*/




