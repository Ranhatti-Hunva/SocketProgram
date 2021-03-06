#include "serverchat.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include "msgqueue.h"
//-----------------------------------------------------------------------------
#define HOST "localhost"
#define PORT "8096"
//-----------------------------------------------------------------------------
void initClientList(std::vector<clientNode> &clientLst){
    for(int i = 0 ; i < MAX_CLIENT ; i++){
        clientLst[i].name = "";
        clientLst[i].status = false;
        clientLst[i].socketfd = -1;
        clientLst[i].id = -1;
        clientLst[i].readfile = false;
    }
}

//-----------------------------------------------------------------------------
int main()
{
    std:: queue <sendNode> qMsgSend;

    thread_pool poolThr{10};
    std::vector<timeoutNode> timeoutList;
    std::vector<clientNode> client(MAX_CLIENT);
    msgQueue qSend;
    ServerChat server(HOST,PORT);
    std::mutex mtx;
    server.initSet();
    int socket = server.createSocket();

    if(socket < 0){
        perror("socket :");
    }

    if(server.listenSocket(socket,BACKLOG) == true){
        std::cout<<"Server started!!!\n";
        initClientList(ref(client));

        long arg = fcntl(socket, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(socket, F_SETFL, arg);


        poolThr.enqueue([&]{
            server.sendThread(qSend,mtx);
        });

        poolThr.enqueue([&]{
            server.timeoutThread(ref(client),ref(timeoutList));
        });
        while(1){
        server.recvData(socket,ref(client),poolThr,ref(timeoutList),qSend,mtx);
        usleep(1000);
        }
        server.cleanUp();
    }
    return 0;
}
