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
#include <queue>
class msg_queue{
    std::mutex msg_mutex;
    std::queue<std::string> msg_waiting;
public:
    void push(std::string str);
    void clear();
    void pop();
    bool empty();
    std::string get();
};

#endif // USERCOMAND_H
