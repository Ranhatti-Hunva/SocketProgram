#ifndef CSQUEUE_H
#define CSQUEUE_H

#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include "handlemsg.h"

struct nodeConsole{
    std::string name;
    std::string data;

};

class csQueue
{
    std::mutex mtxMsg;
    std::queue<nodeConsole> qSend;
public:
    csQueue();
    void pushQ(const nodeConsole node);
    void popQ();
    bool isEmpty();
    void cleadQ();
    int size();
    nodeConsole frontQ();
};

#endif // CSQUEUE_H
