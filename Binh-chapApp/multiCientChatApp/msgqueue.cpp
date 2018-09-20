#include "msgqueue.h"

msgQueue::msgQueue()
{

}

void msgQueue::pushQ(const msg_text node,std::mutex &mt){
    std::lock_guard<std::mutex> locker(mt);
    qSend.push(node);
};


void msgQueue::popQ(std::mutex &mt){
    std::lock_guard<std::mutex> locker(mt);
    qSend.pop();
};

void msgQueue::cleadQ(std::mutex &mt){
    std::lock_guard<std::mutex> locker(mt);
    while(!qSend.empty()){
        qSend.pop();
    }
}

uint msgQueue::size(){    
    return qSend.size();
}

bool msgQueue::isEmpty(std::mutex &mt){
    std::lock_guard<std::mutex> q_msg_send_locker(mt);
    return qSend.empty();
}


msg_text msgQueue::frontQ(std::mutex &mt){
    std::lock_guard<std::mutex> q_msg_send_locker(mt);
    return qSend.front();
};
