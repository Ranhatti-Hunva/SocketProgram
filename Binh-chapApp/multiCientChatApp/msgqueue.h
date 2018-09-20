#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <iostream>
#include "handlemsg.h"


class msgQueue
{
private:
    std::queue<msg_text> qSend;
public:
    msgQueue();
    void pushQ(const msg_text node,std::mutex &mt);
    void popQ(std::mutex &mt);
    bool isEmpty(std::mutex &mt);
    void cleadQ(std::mutex &mt);
    uint size();
    msg_text frontQ(std::mutex &mt);
};

#endif // MSGQUEUE_H
