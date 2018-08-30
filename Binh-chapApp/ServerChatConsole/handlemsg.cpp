#include "handlemsg.h"

HandleMsg::HandleMsg()
{

}

void HandleMsg::ultoc(unsigned int& ul, unsigned char* cu){
    ul = 0;
    for (unsigned int i=0; i<4; i++)
    {
        unsigned int j = static_cast<unsigned int>(*cu++) << i*8;
        ul |= j;
    };
}

bool HandleMsg::packed_msg(struct msg_text& msg_input, unsigned char* buffer){
    unsigned int len = msg_input.msg.length()+9;
    unsigned char len_byte[4];
    memcpy(len_byte, &len, 4);

    unsigned char ID_byte[4];
    static unsigned int ID_msg = 1;


    ID_msg = (ID_msg==0)?ID_msg++:ID_msg;

    if (msg_input.type_msg < 3)
    {
        memcpy(ID_byte, &ID_msg, 4);
        msg_input.ID = ID_msg;
    }
    else
    {
        if(msg_input.ID == 0)
        {
            printf("=> Haven't insert ID of the msg you want to send respond");
            return false;
        }
        else
        {
            memcpy(ID_byte, &msg_input.ID, 4);
        }
    };


    for(unsigned int i = 0; i<len; i++)
    {
        if (i<4)
        {
            *buffer = len_byte[i];
        }
        else if (i == 4)
        {
            *buffer = msg_input.type_msg;
        }
        else if (i<9)
        {
            *buffer = ID_byte[i-5];
        }
        else
        {
            if (msg_input.type_msg<2)
            {
                *buffer = static_cast<unsigned char>(msg_input.msg[i-9]);
            }
        };
        buffer++;
    };
    ID_msg++;
    return true;
}

bool HandleMsg::unpacked_msg(struct msg_text& msg_output, unsigned char* buffer, unsigned int num_data){
    if(num_data < 9)
    {
        // Shortest msg has 9 bytes in length.
        return false;
    }
    else
    {
        unsigned int len_msg;
        ultoc(len_msg,buffer);
        printf("=> Length msg: %u \n", len_msg);
        printf("=> Num data: %u \n", num_data);

        if(num_data < len_msg)
        {
            return false;
        }
        else
        {
            buffer +=4;
            msg_output.type_msg = *buffer;

            buffer ++;
            ultoc(msg_output.ID,buffer);

            if (msg_output.type_msg < 2)
            {
                msg_output.msg.clear();
                buffer +=4;

                while((*buffer != 0) && (buffer != nullptr))
                {
                    msg_output.msg = msg_output.msg + static_cast<char>(*buffer);
                    buffer++;
                };
            };
            return true;
        };
    };
}
