#include "serverchat.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
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
    }
}


//-----------------------------------------------------------------------------
int main()
{
    std:: queue <sendNode> qMsgSend;
    thread_pool poolThr{30};
    std::vector<timeoutNode> timeoutList;
    std::vector<clientNode> client(MAX_CLIENT);

    ServerChat server(HOST,PORT);

    server.initSet();
    int socket = server.createSocket();

    if(socket < 0){
        perror("socket :");
    }

    if(server.listenSocket(socket,BACKLOG) == true){
        std::cout<<"Server started!!!\n";        
        initClientList(ref(client));
//        poolThread.enqueue([rspMsg]{
//            clientQSend(rspMsg);
//        });

        long arg = fcntl(socket, F_GETFL, NULL);
        arg |= O_NONBLOCK;
        fcntl(socket, F_SETFL, arg);


        poolThr.enqueue([&]{
            server.sendThread(ref(qMsgSend));
        });

        poolThr.enqueue([&]{
            server.timeoutThread(ref(client),ref(timeoutList));
        });

        while(1){
            server.recvData(socket,ref(qMsgSend),ref(client),poolThr,ref(timeoutList));
        }

        server.cleanUp();
    }

    //server proccess
    //server.mainLoop();





    return 0;
}
