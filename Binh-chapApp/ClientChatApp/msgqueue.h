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
    std::mutex mt;
public:
    msgQueue();
    void pushQ(const msg_text node);
    void popQ();
    bool isEmpty();
    void cleadQ();
    uint size();
    msg_text frontQ();
};

#endif // MSGQUEUE_H
