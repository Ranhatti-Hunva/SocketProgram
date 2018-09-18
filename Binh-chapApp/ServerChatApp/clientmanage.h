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
#include "handlemsg.h"
#include <mutex>
#include <map>
#include <cstddef>       // std::size_t
#include <memory>

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
                            std::mutex &mt);

    void sendMsgToClient(std::vector <clientNode> &clientList,
                         char *msg,
                         int socketfd,
                         std::vector<timeoutNode> &timeoutList,
                         msgQueue &qSend,
                         std::mutex &mt);

    void sendOffClient(msgQueue &qSend,
                       std::mutex &mt,
                       std::vector <clientNode> &clientList);
    void addQsend (int socketfd,
                   std::vector <timeoutNode> &timeoutList,
                   msgQueue &qSend,
                   std::mutex &mt,
                   uint8_t *buffer,
                   uint lenMsg,
                   uint ID);
    void storeMsg (char *name, char *bufSend);
private:    

};

#endif // CLIENTMANAGE_H
