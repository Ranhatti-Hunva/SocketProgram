#include "clientchat.h"
#include "handlemsg.h"
#include <sys/time.h>
#include <chrono>
#include <future>
#include <queue>
#include <iostream>     // std::cout
#include <algorithm>    // std::find_if
#include <vector>       // std::vector
#include <memory>
//----------------------------------------------------------------------------
#define HOST "10.42.0.127"
#define PORT "8096"
#define TIME_OUT 10
//------------variable check---------------------------------------------------
std::mutex mtx;
int stop = 0;
long int timeOut = 5000; //ms
//std::chrono::milliseconds ms;

//-----struct timeout ---------------------------------------------------------
struct timeoutSend{
    long int time;
    msg_text msg;
    int msgId;
};

//------------thread receive msg from server-----------------------------------

void recvMsg(unsigned char *buf,int sockfd,std::vector<timeoutSend>&timeoutQ){
    long int timeRSP = 0;
    struct timeval tp;
    while(stop!=1){

        buf = new unsigned char [2048];
        memset(buf,0,2048);

        int bytesRecv = recv(sockfd,buf,2048,0);


        if(bytesRecv >0){
            struct msg_text msg_get;
            struct msg_text msg_rsp;
            HandleMsg handleMsg;

            std::vector<unsigned char> buffer;
            buffer.insert(buffer.end(),&buf[0],&buf[bytesRecv]);

//            std::cout<<"recv \n";
//            for(int i = 0; i< bytesRecv;i++){
//                std::cout<<(unsigned int)buf[i]<<" ";
//            }

            //bool is_success = handleMsg.unpacked_msg(msg_get, buf, bytesRecv);
            while(buffer.size()>0){
                bool is_success = handleMsg.unpacked_msg(msg_get,buffer);
                if(!is_success){
                    break;
                }
                else{
                    std::cout<<"\n";
                    // gui rsp to

//                    if(is_success){
//                        std::cout<<"id "<<msg_get.ID<<" type "<<(int)msg_get.type_msg
//                                <<" content "<<msg_get.msg<<"\n";
//                    }
                    if(is_success && msg_get.type_msg != RSP){
                        unsigned char buffer[10];
                        msg_rsp.ID = msg_get.ID;
                        msg_rsp.type_msg = RSP;
                        handleMsg.packed_msg(msg_rsp,buffer);
                        send(sockfd,buffer,9,0);
                        //std::vector<timeoutSend>::iterator it;
                    }
                    if(msg_get.type_msg == RSP){
                        //std::cout << "id rps> " << msg_get.ID << std::endl;

                        mtx.lock();
                        std::vector<timeoutSend>::iterator it;
                        // tim va xoa msg trong Q timeout khi nhan respond
                        if(!timeoutQ.empty()){
                            //std::cout << "co vao\n";
                            it = std::find_if(timeoutQ.begin(),timeoutQ.end(),
                                              [=] (timeoutSend const& f) {
                                return (f.msgId == msg_get.ID);
                            });
                            //std::cout<<"it "<<it.base()->msg.msg<<"\n";
                            //tim thay va xoa
                            if (it != timeoutQ.end()){
                                timeoutQ.erase(it);
                            }

                        }
                        mtx.unlock();
                    }
                    if(msg_get.msg.length() > 0){
                        std::cout << "> " << msg_get.msg << std::endl;
                    }

                }
            }

        }
        if(bytesRecv == 0){
            std::cout << "> server is close \n";
            close(sockfd);
            stop = 1;
            break;
            //exit(1);
        }
        if(stop != 0){
            break;
        }
        delete[] buf;
    }


}


//----thread nhan tu console---------------------------------------------------------------------

void cinFromConsole(int socket,std::queue<msg_text>&msgQ){
    //int socket;std::queue<msg_text>msgQ;

    msg_text msgSend;


    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    while(stop!=1){
        fd_set read;
        FD_ZERO(&read);
        FD_SET(0,&read);
        //select 1 cin ready to read input
        if(select(1,&read,nullptr,nullptr,&tv) == -1){
            perror("select :");
        }
        if(FD_ISSET(0,&read)){
            std::string userInput;
            getline(std::cin,userInput);
            //std::cout<<"usr :"<<userInput<<"\n";
            if(strcmp(userInput.c_str(),"#") == 0){
                close(socket);
                stop = 1;
                userInput.clear();
                break;
            }
            else if(userInput.size() >0){

                msgSend.type_msg = MSG;
                msgSend.msg.assign(userInput);

                mtx.lock();
                msgQ.push(msgSend);
                mtx.unlock();

                userInput.clear();
            }
        }
    }
}
//------------thread send msg to server--------------------------------------------
void sendMsg(int socket,std::queue<msg_text>&msgQ,std::vector<timeoutSend>&timeoutQ){
    HandleMsg handleMsg;
    struct timeval tp;
    while(stop!=1){
        if(!msgQ.empty()){
            int buferSize = msgQ.front().msg.length()+9;

            //unsigned char *buf = new unsigned char [buferSize];
            std::unique_ptr <unsigned char> buf (new unsigned char [buferSize]);
            handleMsg.packed_msg(msgQ.front(),buf.get());

//            std::cout<<"send buf\n";
//            for(int i = 0; i< buferSize;i++){
//                std::cout<<(unsigned int)buf.get()[i]<<" ";
//            }
//            std::cout<<"\n";

            if(send(socket,buf.get(),buferSize,0) > 0){

                timeoutSend node;
                node.msg = msgQ.front();
                node.msgId = msgQ.front().ID;
                gettimeofday(&tp, nullptr);
                node.time = tp.tv_sec*1000 +tp.tv_usec/1000;
                mtx.lock();
                timeoutQ.push_back(node);
                mtx.unlock();
                msgQ.pop();
            }
            else{
                perror("send: ");
            }

        }
    }
}
//-----send ping-------------
void sendPing(int socket){
    HandleMsg handleMsg;
    msg_text ping;
    ping.type_msg = PIG;
    std::unique_ptr <unsigned char> buf (new unsigned char [9]);
    //unsigned char *buf = new unsigned char[9];
    handleMsg.packed_msg(ping,buf.get());
    send(socket,buf.get(),9,0);
    //delete[]buf;
}

//------------thread timeout msg --------------------------------------------------
void timeoutThread(int socket,std::vector<timeoutSend>&timeoutQ,std::queue<msg_text>&msgQ){
    struct timeval tp;
    HandleMsg handleMsg;
    std::unique_ptr <unsigned char> buf (new unsigned char [9]);
    //unsigned char *buf;
    msg_text msgRecv;
    while(stop != 1){
        if(!timeoutQ.empty()){
            gettimeofday(&tp, nullptr);
            if((tp.tv_sec * 1000 + tp.tv_usec / 1000) - timeoutQ.front().time> timeOut){
                std::cout<<"timeout!!!\n";
                gettimeofday(&tp, nullptr);
                long int timePing = tp.tv_sec * 1000 + tp.tv_usec / 1000;
                //ping and wait rsp from server
                mtx.lock();
                sendPing(socket);
                usleep(100000);
                while(1){
                    gettimeofday(&tp, nullptr);
                    if(tp.tv_sec * 1000 + tp.tv_usec / 1000 - timePing>timeOut*2){
                        std::cout<<"can't connect to server\n";
                        close(socket);
                        stop = 1;
                        break;
                    }
                    //buf = new unsigned char [9];
                    int numRecv = recv(socket,buf.get(),9,0);
                    if(numRecv > 0){

                        std::vector<unsigned char> buffer;
                        buffer.insert(buffer.end(),&buf.get()[0],&buf.get()[numRecv]);
                        while(buffer.size()>0){
                            bool is_success = handleMsg.unpacked_msg(msgRecv,buffer);
                            if(!is_success){
                                break;
                            }
                            else{
                                if(msgRecv.type_msg == RSP){
                                    // continue chat resend msg erease msg in timeoutQ

                                    //resend msg;
                                    msgQ.push(timeoutQ.front().msg);

                                    timeoutQ.erase(timeoutQ.begin());


                                    break;
                                }
                            }
                        }
                        //handleMsg.unpacked_msg(msgRecv,buf.get(),numRecv);

                    }
                    else if(numRecv == 0){
                        close(socket);
                        std::cout<<"server is close!!!\n";
                        stop = 1;
                        break;
                    }

                }
                mtx.unlock();
            }

        }
    }
}

//---------------------------------------------------------------------------------
std::string getLineFromCin() {
    std::string line;
    std::getline(std::cin, line);
    return line;
}
//------------reconnect function---------------------------------------------------
bool isReconnect(std::vector<timeoutSend>&timeoutQ){
    std::string ans;
    while(!timeoutQ.empty()){
        std::cout <<"content " <<timeoutQ.back().msg.msg<<"\n";
        std::cout <<"id " <<timeoutQ.back().msgId<<"\n";
        std::cout <<"time " <<timeoutQ.back().time<<"\n";
        timeoutQ.pop_back();
    }
    while(1){
        std::cout << "Do you want reconnect to server ? (Y/N)\n";

        getline(std::cin,ans);

        if(ans.compare("Y") == 0){
            std::cin.clear();
            return true;
        }
        else if(ans.compare("N") == 0){
            std::cin.clear();
            return false;
        }
        else{
            std::cout << "Your command is not found, try again!!!\n";
        }
    }
}

//------------main----------------------------------------------------------
int main()
{

    struct msg_text msgSend;
    msgSend.type_msg = SGI;
    ClientChat client;
    unsigned char * bufrcv; // buf for recv data
    char statusBuf[10]; // buf status login
    std::string userInput;
    std::vector <timeoutSend> timeoutList;
    std:: queue <msg_text> qSend;
    bool reconnect = true;

    while(reconnect){
        stop = 0;
        std::cout << "Your name: ";
        std::string name = "all";

        //user name != all or != null
        while(!name.compare("all"))
        {
            getline(std::cin,name);

            if(name.empty()){
                std::cout<<"Sorry!! You can use this name. Plz re-insert username:\n";
                name = "all";
                //exit(EXIT_FAILURE);

            }
            else if(!name.compare("all")){
                std::cout<<"Sorry!! You can use this name. Plz re-insert username:\n";
            }
        }

        //getline(std::cin,name);

        msgSend.msg.assign(name);
        unsigned char buffer[msgSend.msg.length()+9];
        HandleMsg handleMsg;
        handleMsg.packed_msg(msgSend,buffer);

        //connect to server
        int socket = client.timeoutConnect(HOST,PORT,TIME_OUT);

        //connect success
        if(socket > 0){
            //send name user to server for login
            int num = send(socket,buffer,sizeof(buffer),0);

            //check valid username
            while(1){
                memset(statusBuf,0,10);
                int numRecv = recv(socket,statusBuf,10,0);
                //mtx.lock();
                //wait respond from server
                if(numRecv >0){
                    std::cout << "Login success\n";
                    std::cout << "Start chat now" <<"\n";
                    std::cout  << "press '#' to exit\n";
                    break;
                }
            }

            //socket non-blocking mode
            fcntl(socket, F_SETFL, O_NONBLOCK);


            //thread receive msg



            std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend));
            std::thread sendMsgThread(sendMsg,socket,ref(qSend),ref(timeoutList));
            std::thread recvThread(recvMsg,bufrcv,socket,ref(timeoutList));
            std::thread timeoutThr(timeoutThread,socket,ref(timeoutList),ref(qSend));
            // main thread for send msg
            // press # for logout
            //auto future = std::async(std::launch::async, getLineFromCin);
            /*while(1){

                getline(std::cin,userInput);


                //std::string line;
                //if (future.wait_for(std::chrono::seconds(1)) == std::future_status::ready) {
                //    std::cout  << ">";
                //    userInput = future.get();
                //    future = std::async(std::launch::async, getLineFromCin);

                //}


                if(strcmp(userInput.c_str(),"#") == 0){
                    send(socket,"",0,0);
                    close(socket);
                    //end_connection = true;
                    stop = 1;
                    //std::terminate();
                    userInput.clear();
                    break;
                }
                else if(userInput.size() >0){

                    msgSend.type_msg = MSG;
                    msgSend.msg.assign(userInput);

                    unsigned char bufSend[userInput.size()+9];
                    memset(bufSend,0,userInput.size()+9);
                    handleMsg.packed_msg(msgSend,bufSend);

                    int sendResult = send(socket,bufSend,sizeof(bufSend),0);

                    //set variable for timeout
                    mtx.lock();
                    struct timeval tp;
                    gettimeofday(&tp, nullptr);

                    timeoutSend node;
                    node.msgId = msgSend.ID;
                    node.time = (tp.tv_sec * 1000 + tp.tv_usec / 1000);
                    node.msg = msgSend;
                    timeoutList.push_back(node);


                    mtx.unlock();

                    if(sendResult == -1){
                        perror("send");
                        break;
                    }
                    userInput.clear();
                }

            }*/

            //stop all thread before reconnect
            sendMsgThread.join();
            recvThread.join();
            cinConsoleThread.join();
            timeoutThr.join();

        }

        reconnect = isReconnect(ref(timeoutList));

    }
    return 0;
}
