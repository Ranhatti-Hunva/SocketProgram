#include "msgqueue.h"

// Message.
std::string msg_queue::msg_get()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    return msg_waiting.front();;
}

void msg_queue::push_msg(std::string str)
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    msg_waiting.push(str);
}

void msg_queue::pop_msg()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    msg_waiting.pop();
}

bool msg_queue::msg_empty()
{
    std::lock_guard<std::mutex> guard(msg_mutex);
    return msg_waiting.empty();
}

// Respond.
std::string msg_queue::respond_get()
{
    std::lock_guard<std::mutex> guard_second(respond_muxtex);
    return respond.front();;
}

void msg_queue::push_respond(std::string str)
{
    std::lock_guard<std::mutex> guard_second(respond_muxtex);
    respond.push(str);
}

void msg_queue::pop_respond()
{
    std::lock_guard<std::mutex> guard_second(respond_muxtex);
    respond.pop();
}

bool msg_queue::respond_empty()
{
    std::lock_guard<std::mutex> guard_second(respond_muxtex);
    return respond.empty();
}

// Clear both of 2 queue
void msg_queue::clear()
{
    {
        std::lock_guard<std::mutex> guard(msg_mutex);
        while (!msg_waiting.empty())
        {
            msg_waiting.pop();
        };
    };

    {
        std::lock_guard<std::mutex> guard_second(respond_muxtex);
        while (!respond.empty()){
            respond.pop();
        };
    };
}

