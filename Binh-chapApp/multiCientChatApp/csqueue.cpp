#include "csqueue.h"

csQueue::csQueue()
{

}

void csQueue::pushQ(const nodeConsole node){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    qSend.push(node);
};


void csQueue::popQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    qSend.pop();
};

void csQueue::cleadQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    while(!qSend.empty()){
        qSend.pop();
    }
}

bool csQueue::isEmpty(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    return qSend.empty();
}

int csQueue::size(){
    return qSend.size();
}

nodeConsole csQueue::frontQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    return qSend.front();
};
