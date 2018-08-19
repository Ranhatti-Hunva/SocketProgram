#ifndef USERCOMAND_H
#define USERCOMAND_H

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <vector>

class user_command{
    std::mutex userInputMutex;
    std::string userCommand;
public:
    void set(std::string str);
    void clear();
    bool empty();
    std::string get();
    bool compare(std::string str);
};

#endif // USERCOMAND_H
