#ifndef USERCOMAND_H
#define USERCOMAND_H

#include <string>
#include <string.h>
#include <mutex>
#include <queue>

class msg_queue{
    std::mutex msg_mutex;
    std::mutex respond_muxtex;

    std::queue<std::string> msg_waiting;
    std::queue<std::string> respond;
public:
    void push_msg(const std::string str);
    void push_respond(const std::string str);

    void clear();

    void pop_msg();
    void pop_respond();

    bool msg_empty();
    bool respond_empty();

    std::string msg_get();
    std::string respond_get();
};

#endif // USERCOMAND_H
