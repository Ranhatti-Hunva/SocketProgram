#ifndef MSGQUEUE_H
#define MSGQUEUE_H

#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include "handlemsg.h"


class msgQueue
{
    std::mutex mtxMsg;
    std::queue<sendNode> qSend;
public:
    msgQueue();
    void pushQ(const sendNode node);
    void popQ();
    bool isEmpty();
    void cleadQ();
    sendNode frontQ();
};

#endif // MSGQUEUE_H
