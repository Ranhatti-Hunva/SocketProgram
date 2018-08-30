#include "udphelper.h"

//struct addrinfo* UDPhelper::get_addinfo_list(std::string host_name, int port_num)
//{
//    struct addrinfo* infor_list;        // servinfor is linked-list which contain all address information.
//    long status;                        // Check error from getaddrinfor().Status return no-zero if there's an error.

//    struct addrinfo hints;              // Clue to find the address information.
//    memset(&hints, 0, sizeof(hints));   // To sure that hint is empty.
//    hints.ai_family = AF_INET;          // Fill information for the hint. APF_INET: use IPv4,
//    hints.ai_socktype = SOCK_STREAM;    // SOCK_STREAM: TCP/IP menthod.

//    if(host_name.empty())
//    {
//        hints.ai_flags = AI_PASSIVE;    // AI_PASSIVE: assigned the address of local host tho the socket structure.
//        status = getaddrinfo(nullptr, std::to_string(port_num).c_str(), &hints, &infor_list);
//    }
//    else
//    {
//        status = getaddrinfo(host_name.c_str(), std::to_string(port_num).c_str(), &hints, &infor_list);
//    };

//    if (status!=0)
//    {
//        fprintf(stderr, "=> getaddinfo() error: %s", gai_strerror(static_cast<int>(status)));
//        fprintf(stderr, "%d", static_cast<int>(status));
//        exit(EXIT_FAILURE);
//    };
//    return infor_list;
//};

//void UDPhelper::operator()(int size_of_file)
//{
//    struct addrinfo *p;
//    p = get_addinfo_list("", 2000);

//    int listerner_fd;
//    if ((listerner_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
//    {
//        perror("=> Socket failed");
//        exit(EXIT_FAILURE);
//    };

//    if ((::bind(listerner_fd, p->ai_addr, p->ai_addrlen))<0)
//    {
//        perror("=> Bind failed");
//        exit(EXIT_FAILURE);
//    };

//    struct sockaddr_in *addr_used = reinterpret_cast<struct sockaddr_in *>(p->ai_addr); // Show the IP infor to terminal
//    void *addr_infor = &(addr_used -> sin_addr);
//    inet_ntop(p->ai_family, addr_infor, ipstr, sizeof(ipstr));
//    printf("=> Created new UDP socket with IP %s - port 2000\n", ipstr);

//    struct sockaddr_in talker_addr;
//    unsigned int size_talker_address = sizeof(talker_addr);
//    memset(&talker_addr, 0, size_talker_address);

//    char buffer[bufsize] = {0};

//    off_t numbyte, count;
//    count = 0;

//    std::chrono::system_clock::time_point timeout;
//    while(count < size_of_file)
//    {
//        numbyte = recvfrom(listerner_fd, buffer,
//                           bufsize, 0,
//                           ( struct sockaddr *) &talker_addr,
//                           &size_talker_address);
//    }
//}

//void UDPhelper::operator()(std::string& UDP_addr,std::string filename)
//{};



