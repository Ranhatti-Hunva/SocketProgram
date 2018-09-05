#include "serverchat.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
//-----------------------------------------------------------------------------
#define HOST "10.42.0.127"
#define PORT "8096"


int main()
{

    ServerChat server(HOST,PORT);

    server.initSet();

    server.listenSocket(server.createSocket(),BACKLOG);

    //server proccess
    server.mainLoop();

    server.cleanUp();
    return 0;
}
