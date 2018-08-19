#include "tcphelper.h"

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


//int TCPhelper::creat_socket_fd(int doimain, int type, int protocol){};
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




