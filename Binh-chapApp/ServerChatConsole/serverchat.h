#ifndef SERVERCHAT_H
#define SERVERCHAT_H
//-----Library-----------------------------------------------------------------
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <string>
#include <cstring>
#include <map>
#define BACKLOG 10

//-----Class Modify------------------------------------------------------------

class ServerChat
{
public:
    ServerChat(char* ipAddress, char* port);
    ~ServerChat();
    void initSet();
    int createSocket();
    void mainLoop();
    bool listenSocket(int sock, int backLog);
    void *get_in_addr(struct sockaddr *sa);
    void cleanUp();
private:
    fd_set master; //master file decriptor list
    fd_set read_fds; //temp file descritopr list for select
    int sockfd,fdmax, newfd; // sockfd socket file descriptor, listening onl sockfd
    // max number of connection on fdmax
    // new file descritor
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];
    int rv;

    char* m_ipAddr;
    char* m_port;

    char buf[4096];    // buffer for client data
    int nbytes;

    char remoteIP[INET6_ADDRSTRLEN];
};

#endif // SERVERCHAT_H
