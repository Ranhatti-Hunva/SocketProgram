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

    //std::cout << "number = \n";
    ServerChat server("localhost","8096");
    //std::cout << "number = 1\n";
    server.initSet();
    //std::cout << "number = 2\n";
    server.listenSocket(server.createSocket(),BACKLOG);
    //std::cout << "number = 3\n";
    server.mainLoop();
    //std::cout << "number = 4\n";
    server.cleanUp();
    return 0;
}
