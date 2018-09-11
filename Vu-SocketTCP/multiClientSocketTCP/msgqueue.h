#ifndef USERCOMAND_H
#define USERCOMAND_H

#include <string>
#include <string.h>
#include <vector>
#include <mutex>
#include <queue>

#define Q_MSG 0
#define Q_RSP 1

using namespace std;
struct msg_text
{
    string msg;
    unsigned char type_msg;
    unsigned int ID = 0;
};

class msg_queue{
    std::mutex msg_mutex;
    std::mutex rsp_mutex;

    std::queue<std::vector<unsigned char>> msg;
    std::queue<std::vector<unsigned char>> rsp;

public:
    void push(const std::vector<unsigned char> element, int type_queue);

    void clear(int type_queue);

    void pop(int type_queue);

    bool is_empty(int type_queue);

    std::vector<unsigned char> get(int type_queue);
};

#endif // USERCOMAND_H
