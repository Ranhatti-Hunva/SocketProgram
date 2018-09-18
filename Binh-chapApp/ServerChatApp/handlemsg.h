#ifndef HANDLEMSG_H
#define HANDLEMSG_H
#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <vector>



#define NOT_RSP 0
#define SGI 0
#define MSG 1
#define PIG 2
#define RSP 3


struct msg_text
{
    std::string msg;
    unsigned char type_msg;
    unsigned int ID = 0;
    int socketfd = -1;
};

struct sendNode
{
    int socket;
    unsigned char * buf;
    int len;
    int msgID;
};

class HandleMsg
{
public:    
    HandleMsg();
    void ultoc(unsigned int& ul, unsigned char* cu);
    bool packed_msg(struct msg_text& msg_input, unsigned char* buffer);    
    //bool unpacked_msg(struct msg_text& msg_output, unsigned char* buffer, unsigned int num_data);
    bool unpacked_msg(msg_text& msg_output, std::vector<unsigned char>& buffer);

};

#endif // HANDLEMSG_H
