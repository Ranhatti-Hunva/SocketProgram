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
#include <map>
#include <thread>
#include <pthread.h>
#include <sys/fcntl.h>
#include <vector>
#include <netinet/tcp.h>
#include <queue>
#include <mutex>
#include <sstream>
#include "handlemsg.h"
#include "clientmanage.h"
#include "clientnode.h"
#include "threadpool.h"

#define BACKLOG 100
//#define MAX_CLIENT 50


//-----struct Modify------------------------------------------------------------
//struct msgType{
//    int time;
//    char *msg;
//    msgType *next;
//};

//-----Class Modify------------------------------------------------------------
class ServerChat
{
    //std::mutex client_mutext;
protected:

public:
    ServerChat(char* ipAddress, char* port);
    ~ServerChat();
    void initSet();
    int createSocket();
    void mainLoop();
    bool listenSocket(int sock, int backLog);
    void *get_in_addr(struct sockaddr *sa);
    void cleanUp();
    void clientQRecv(struct msg_text msgHandle, std::vector<clientNode> &clientList, std::vector<timeoutNode> &timeoutList);
    //std::vector<clientNode> client();
    void timeoutThread(fd_set &fd,std::vector <clientNode> &clientList, std::vector <timeoutNode> &timeoutList);


private:
    long int timeOut = 20000;
    fd_set listener; //listener file decriptor list
    fd_set read_fds; //temp file descritopr list for select
    int sockfd,fdmax, newfd; // sockfd socket file descriptor, listening onl sockfd
    // max number of connection on fdmax
    // new file descritor
    //std:: queue <struct msg_>  qRecv;
    ThreadPool pool{30};

    std:: queue <struct msg_text> qRecv;
    std:: queue <struct msg_text> qSend;


    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage remoteaddr; // client address
    socklen_t addrlen;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];

    HandleMsg handlMsg;
    int rv;

    char* m_ipAddr;
    char* m_port;

     /*= new unsigned char[1024]*/;    // buffer for client data
    int nbytes;
    int skExist = -1;

    char remoteIP[INET6_ADDRSTRLEN];
    std::mutex mtx;
};

#endif // SERVERCHAT_H
