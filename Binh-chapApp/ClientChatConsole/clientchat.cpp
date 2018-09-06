#include "clientchat.h"

ClientChat::ClientChat()
//    :m_svAddr(svAddr),m_port(port)
{
    FD_ZERO(&talker);
    fd_max = 0;
}

ClientChat::~ClientChat()
{
    cleanUp();
}

void * ClientChat::get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


void ClientChat::initClient(){
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(m_svAddr, m_port, &hints, &servinfo)) != 0) {
        std::cerr<<"getaddrinfo: " << gai_strerror(rv) << "\n";
        return;
    }
}


int ClientChat::createSocket(){
    for( p = servinfo; p!= nullptr; p = p->ai_next){
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("client: socket");
            continue;
        }

        if(connect(sockfd,p->ai_addr,p->ai_addrlen) == -1){
            close(sockfd);
            perror("client: connect");
            return -1;
        }

        break;
    }

    if(p == nullptr){
        std :: cerr << "client failed to connect";
        return -2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);

    freeaddrinfo(servinfo);

    std::cout << "client connecting to" << s <<"\n";

    return sockfd;
}


int ClientChat::sendall(int socket,const char *buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(socket, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}


void ClientChat::mainLoop(){
}

int ClientChat::getsocket(){
    return sockfd;
}

int  ClientChat::timeoutConnect(char *host,char* port, int timeout){
    timeval Timeout;
    Timeout.tv_sec = timeout;
    Timeout.tv_usec = 0;
    fd_set Write, Err;
    socklen_t lon;
    int valopt;
    //std::cout <<" co vao \n";
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        std::cerr<<"getaddrinfo: " << gai_strerror(rv) << "\n";
        return -1;
    }

    for( p = servinfo; p!= nullptr; p = p->ai_next){
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("client: socket");
            continue;
        }

        long arg = fcntl(sockfd, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(sockfd, F_SETFL, arg);

        int connectStatus = connect(sockfd,p->ai_addr,p->ai_addrlen);
        if (connectStatus < 0) {

            if (errno == EINPROGRESS) {
                fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
                do {

                    FD_ZERO(&Write);
                    FD_SET(sockfd, &Write);
                    connectStatus = select(sockfd+1, nullptr, &Write, nullptr, &Timeout);
                    if (connectStatus < 0 && errno != EINTR) {
                        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                        return -1;
                    }
                    else if (connectStatus > 0) {
                        // Socket selected for write
                        lon = sizeof(int);
                        if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) {
                            fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
                            return -1;
                        }
                        // Check the value returned...
                        if (valopt) {
                            fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)
                                    );
                            return -1;
                        }
                        break;
                    }
                    else {
                        fprintf(stderr, "Timeout in select() - Cancelling!\n");
                        return -1;
                    }
                } while (1);
            }
            else {
                fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
                return -1;
            }

//            arg = fcntl(sockfd, F_GETFL, NULL);
//            arg &= (~O_NONBLOCK);
//            fcntl(sockfd, F_SETFL, arg);

            return sockfd;
        }

    }
}

std::string ClientChat::getServerName(){
    return std::string(s,0,sizeof(s));
}


void ClientChat::cleanUp(){
    close(sockfd);
}
