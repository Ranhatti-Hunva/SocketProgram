#include "msgqueue.h"

void msg_queue::push(const std::vector<unsigned char> element, int type_queue)
{
    if (type_queue > 1)
    {
        perror("=> Wrong type queue!");
    }
    else
    {
        switch(type_queue)
        {
            case Q_MSG:
            {
                std::lock_guard<std::mutex> q_msg_send_locker(msg_mutex);
                msg.push(element);
                break;
            };
            case Q_RSP:
            {
                std::lock_guard<std::mutex> q_rsp_locker(rsp_mutex);
                rsp.push(element);
                break;
            };
        };
    };
};

void msg_queue::clear(int type_queue)
{
    if (type_queue > 2)
    {
        perror("=> Wrong type queue!");
    }
    else
    {
        switch(type_queue)
        {
            case Q_MSG:
            {
                std::lock_guard<std::mutex> q_msg_send_locker(msg_mutex);
                while (!msg.empty())
                {
                    msg.pop();
                };
                break;
            };
            case Q_RSP:
            {
                std::lock_guard<std::mutex> q_rsp_locker(rsp_mutex);
                while (!rsp.empty())
                {
                    rsp.pop();
                };
                break;
            };
        };
    };
};

void msg_queue::pop(int type_queue)
{
    if (type_queue > 1)
    {
        perror("=> Wrong type queue!");
    }
    else
    {
        switch(type_queue)
        {
            case Q_MSG:
            {
                std::lock_guard<std::mutex> q_msg_send_locker(msg_mutex);
                msg.pop();
                break;
            };
            case Q_RSP:
            {
                std::lock_guard<std::mutex> q_rsp_locker(rsp_mutex);
                rsp.pop();
                break;
            };
        };
    };
};

bool msg_queue::is_empty(int type_queue)
{
    if (type_queue > 2)
    {
        perror("=> Wrong type queue!");
        return false;
    }
    else
    {
        switch(type_queue)
        {
            case Q_MSG:
            {
                std::lock_guard<std::mutex> q_msg_send_locker(msg_mutex);
                return msg.empty();
            };
            case Q_RSP:
            {
                std::lock_guard<std::mutex> q_rsp_locker(rsp_mutex);
                return rsp.empty();
            };
        };
        return false;
    };
};

std::vector<unsigned char> msg_queue::get(int type_queue)
{
    std::vector<unsigned char> element;
    element.clear();

    if (type_queue > 2)
    {
        perror("=> Wrong type queue!");
        return element;
    }
    else
    {
        switch(type_queue)
        {
            case Q_MSG:
            {
                std::lock_guard<std::mutex> q_msg_send_locker(msg_mutex);
                element = msg.front();
                break;
            };
            case Q_RSP:
            {
                std::lock_guard<std::mutex> q_rsp_locker(rsp_mutex);
                element = rsp.front();
                break;
            };
        };
        return element;
    };
};

