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
#define MAX_CLIENT 10

class ClientManage
{
public:
    ClientManage();
    void mapClientWithSocket(std::vector <clientNode> &clientList,int socketfd,char * buf);
    void sendMsgToClient(std::vector <clientNode> &clientList,char *msg, int socketfd);
};

#endif // CLIENTMANAGE_H
