#ifndef USERCOMAND_H
#define USERCOMAND_H

#include <string>
#include <string.h>
#include <mutex>
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
