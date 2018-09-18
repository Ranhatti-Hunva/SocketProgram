#include "handlemsg.h"

HandleMsg::HandleMsg()
{

}
//---------------------------------------------------------------------------------------
void HandleMsg::ultoc(unsigned int& ul, unsigned char* cu){
    ul = 0;
    for (unsigned int i=0; i<4; i++)
    {
        unsigned int j = static_cast<unsigned int>(*cu++) << i*8;
        ul |= j;
    };
}
//---------------------------------------------------------------------------------------
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

//---------------------------------------------------------------------------------------
bool HandleMsg::unpacked_msg(msg_text& msg_output, std::vector<unsigned char>& buffer)
{
    if(buffer.size() < 9)
    {
        // Shortest msg has 9 bytes in length.
        return false;
    }
    else
    {
        unsigned int len_msg;
        unsigned char len_msg_c[4] = {buffer[0],buffer[1],buffer[2],buffer[3]};
        ultoc(len_msg,len_msg_c);

        if(buffer.size() < len_msg)
        {
            return false;
        }
        else
        {
            msg_output.type_msg = buffer[4];

            unsigned char ID_msg_c[4] = {buffer[5],buffer[6],buffer[7],buffer[8]};
            ultoc(msg_output.ID,ID_msg_c);

            if (msg_output.type_msg < 2)
            {
                msg_output.msg.clear();

                for(unsigned long i=9; i<len_msg; i++)
                {
                    msg_output.msg = msg_output.msg + static_cast<char>(buffer[i]);
                };
            };

            std::vector<unsigned char> buffer_remain(buffer.begin()+len_msg, buffer.end());
            buffer.clear();
            buffer = buffer_remain;

            return true;
        };
    };
};

