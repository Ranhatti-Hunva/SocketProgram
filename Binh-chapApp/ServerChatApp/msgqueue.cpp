#include "msgqueue.h"

msgQueue::msgQueue()
{

}

void msgQueue::pushQ(const sendNode node){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    qSend.push(node);
};


void msgQueue::popQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    qSend.pop();
};

void msgQueue::cleadQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    while(!qSend.empty()){
        qSend.pop();
    }
}

bool msgQueue::isEmpty(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    return qSend.empty();
}


sendNode msgQueue::frontQ(){
    std::lock_guard<std::mutex> q_msg_send_locker(mtxMsg);
    return qSend.front();
};
