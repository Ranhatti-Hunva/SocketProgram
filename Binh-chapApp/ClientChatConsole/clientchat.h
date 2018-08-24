#ifndef CLIENTCHAT_H
#define CLIENTCHAT_H

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/unistd.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <thread>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <iomanip>
#include <stdio.h>

#define PORT "8096" // the port client will be connecting to

#define MAXDATASIZE 4096 // max number of bytes we can get at once

class ClientChat
{
public:
    ClientChat(char * svAddr, char * port);
    ~ClientChat();
    void *get_in_addr(struct sockaddr *sa);
    void initClient();
    // createsocket and connect
    int createSocket();
    void mainLoop();
    void cleanUp();
    int sendall(int socket, const char *buf,int len);
private:
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    char * m_port;
    char * m_svAddr;

    int rv;
    char s[INET6_ADDRSTRLEN];
};

#endif // CLIENTCHAT_H
