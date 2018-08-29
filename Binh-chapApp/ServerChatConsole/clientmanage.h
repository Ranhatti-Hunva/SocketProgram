#ifndef CLIENTMANAGE_H
#define CLIENTMANAGE_H
#include "serverchat.h"
#include <fstream>
#include <iostream>
#include <time.h>
class ClientManage
{
public:
    ClientManage();
    void mapClientWithSocket(clientNode clientList[],int socketfd,char *buf,fd_set listener);
    void sendMsgToClient(clientNode clientList[],char *msg, int socketfd);
};

#endif // CLIENTMANAGE_H
