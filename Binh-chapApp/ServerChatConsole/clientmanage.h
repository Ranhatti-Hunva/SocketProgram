#ifndef CLIENTMANAGE_H
#define CLIENTMANAGE_H
#include "clientnode.h"
#include <fstream>
#include <iostream>
#include <time.h>
#include <sys/fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <handlemsg.h>
#include <sys/time.h>
#include "msgqueue.h"
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <cstddef>        // std::size_t

#include <memory>
#include <mutex>
#define MAX_CLIENT 100



class ClientManage
{
    std::mutex bfSend;
public:
    ClientManage();
    int mapClientWithSocket(std::vector <clientNode> &clientList,
                            int socketfd,
                            char * buf,
                            fd_set &fds,
                            std:: queue <sendNode> &qMsgSend, msgQueue &qSend);
    void sendMsgToClient(std::vector <clientNode> &clientList,
                         char *msg, int socketfd,
                         std::vector<timeoutNode> &timeoutList,
                         std::queue<sendNode> &qMsgSend,msgQueue &qSend);
private:
    //std::mutex mtx;
};

#endif // CLIENTMANAGE_H
