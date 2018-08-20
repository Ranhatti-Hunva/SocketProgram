#include "msgqueue.h"

void msg_queue::push(std::string str)
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    msg_waiting.push(str);
}

void msg_queue::clear()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    while (!msg_waiting.empty())
    {
        msg_waiting.pop();
    }
}

bool msg_queue:: empty()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    return msg_waiting.empty();
}

std::string msg_queue::get()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    return msg_waiting.front();;
}

void msg_queue::pop()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    msg_waiting.pop();
}
