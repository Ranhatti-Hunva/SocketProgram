#ifndef CLIENTNODE_H
#define CLIENTNODE_H
#include <iostream>
#include <queue>
#include <string>
//-----struct Modify------------------------------------------------------------
struct clientNode{
    char *name;
    bool status;
    int socketfd;
    std:: queue <std::string> msg;
    int id;
};
#endif // CLIENTNODE_H
