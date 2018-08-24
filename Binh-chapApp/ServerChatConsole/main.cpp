#include "serverchat.h"


int main()
{
    //    // init

    //    // create db
    //    // connect Db show all client name - status on-off
    //    // listen

    ServerChat server("localhost","8096");
    server.initSet();
    server.listenSocket(server.createSocket(),BACKLOG);
    server.mainLoop();
    server.cleanUp();
    return 0;
}
