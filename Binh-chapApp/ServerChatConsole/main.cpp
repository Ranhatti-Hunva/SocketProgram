#include "serverchat.h"
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

int main()
{
    //    // init

    //    // create db
    //    // connect Db show all client name - status on-off
    //    // listen

    //    std::ostringstream  number_str;
    //        int number = 25;

    //        number_str << number;
    //        std::cout << "number = '" << number_str.str() << "'" << std::endl;


    ServerChat server("localhost","8096");
    server.initSet();
    server.listenSocket(server.createSocket(),BACKLOG);
    server.mainLoop();
    server.cleanUp();
    return 0;
}
