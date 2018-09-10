#ifndef USERCOMAND_H
#define USERCOMAND_H

#include <string>
#include <string.h>
#include <vector>
#include <mutex>
#include <queue>

#define Q_MSG 0
#define Q_RSP 1
#define Q_RECV 2

using namespace std;
struct msg_text
{
    string msg;
    unsigned char type_msg;
    unsigned int ID = 0;
};

struct q_element
{
    std::vector<unsigned char> content;
    int socket_fd;
};

class msg_queue{

    std::mutex msg_mutex;
    std::mutex rsp_mutex;
    std::mutex recv_mutex;

    std::queue<q_element> msg;
    std::queue<q_element> rsp;
    std::queue<q_element> recv;
public:
    void push(const q_element element, int type_queue);

    void clear(int type_queue);

    void pop(int type_queue);

    bool is_empty(int type_queue);

    q_element get(int type_queue);
};

#endif // USERCOMAND_H
