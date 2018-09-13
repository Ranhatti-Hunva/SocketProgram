#ifndef CLIENTNODE_H
#define CLIENTNODE_H
#include <iostream>
#include <queue>
#include <string>
#include <vector>

//-----struct Modify------------------------------------------------------------
struct clientNode{
    char *name;
    bool status;
    int socketfd;
    std:: queue <std::string> msg;
    int id;
    bool readfile;
};

struct timeoutNode{
    int socket;
    int msgID;
    long int timeout;
};

struct nodeRecv{
    int socket;
    std::queue <uint8_t> data;
};

#endif // CLIENTNODE_H
