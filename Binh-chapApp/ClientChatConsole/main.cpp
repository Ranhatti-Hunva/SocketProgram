#include "clientchat.h"
#include "handlemsg.h"
#include <sys/time.h>
#include <chrono>
#include <future>
#include <queue>
//----------------------------------------------------------------------------
#define HOST "localhost"
#define PORT "8096"
#define TIME_OUT 10
//------------variable check---------------------------------------------------
std::mutex mtx;
int stop = 0;
long int ms = 0;
long int timeOut = 5000; //ms
//std::chrono::milliseconds ms;
//------------thread receive msg from server-----------------------------------

void recvMsg(unsigned char *buf,int sockfd){
    long int timeRSP = 0;
    struct timeval tp;
    while(stop!=1){

        if(ms >0){
            gettimeofday(&tp, nullptr);
            timeRSP = (tp.tv_sec * 1000 + tp.tv_usec / 1000)-ms;
        }

        if(timeRSP>timeOut){
            std::cout<<"not recv rsp from server\n";
            std::cout<<"time wait "<<timeRSP<<" ms\n";
            std::cout<<"still wait 10 s if not recv rsp - close socket\n";
            gettimeofday(&tp, nullptr);

            long int time = tp.tv_sec * 1000 + tp.tv_usec;
            // end_connection = true;
            // resend msg
            mtx.lock();
            struct msg_text msg_ping;
            struct msg_text msg_rsp;
            HandleMsg handleMsg;
            msg_ping.type_msg = PIG;
            unsigned char buffer[10];
            handleMsg.packed_msg(msg_ping,buffer);

            send(sockfd,buffer,10,0);
            unsigned char *buf1 = new unsigned char [100];
            memset(buf1,0,100);
            while(1){
                if(time >0){
                    gettimeofday(&tp, nullptr);
                    timeRSP = (tp.tv_sec * 1000 + tp.tv_usec / 1000)-time;
                }
                if(timeRSP> timeOut*2){
                    std::cout<<"not recv rsp from server - close socket\n";
                    close(sockfd);
                    exit(1);
                }


                int bytesRecv = recv(sockfd,buf1,100,0);
                if(bytesRecv>0){
                    handleMsg.unpacked_msg(msg_rsp,buf1,bytesRecv);
                    if(msg_rsp.ID+1 == msg_ping.ID){
                        timeRSP = 0;
                        ms = 0;
                        std::cout<<"continue chat\n>";
                        break;
                    }
                }
            }
            mtx.unlock();
        }

        buf = new unsigned char [2048];
        memset(buf,0,2048);

        int bytesRecv = recv(sockfd,buf,2048,0);


        if(bytesRecv >0){
            struct msg_text msg_get;
            struct msg_text msg_rsp;
            HandleMsg handleMsg;
            bool is_success = handleMsg.unpacked_msg(msg_get, buf, bytesRecv);
            // gui rsp to

            if(is_success && msg_get.type_msg != RSP){
                unsigned char buffer[10];
                msg_rsp.ID = msg_get.ID;
                msg_rsp.type_msg = RSP;
                handleMsg.packed_msg(msg_rsp,buffer);
                send(sockfd,buffer,9,0);
            }

            if(msg_get.type_msg == RSP){
                timeRSP = 0;
                ms = 0;
                std::cout << "id rps> " << msg_get.ID << std::endl;
            }
            if(msg_get.msg.length() > 0)
                std::cout << "> " << msg_get.msg << std::endl;
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


//-----struct
struct timeoutSend{
  long int time;
  msg_text msg;
  int msgId;
};


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
            std::cout<<"usr :"<<userInput<<"\n";
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
            unsigned char *buf = new unsigned char [buferSize];
            handleMsg.packed_msg(msgQ.front(),buf);
            if(send(socket,buf,buferSize,0) > 0){

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
    unsigned char *buf = new unsigned char[9];
    handleMsg.packed_msg(ping,buf);
    send(socket,buf,9,0);
    delete[]buf;
}

//------------thread timeout msg --------------------------------------------------
void timeoutThread(int socket,std::vector<timeoutSend>&timeoutQ){
    struct timeval tp;
    HandleMsg handleMsg;
    unsigned char *buf;
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
                while(1){
                    gettimeofday(&tp, nullptr);
                    if(tp.tv_sec * 1000 + tp.tv_usec / 1000 - timePing>timeOut*2){
                        std::cout<<"can't connect to server\n";
                        close(socket);
                        stop = 1;
                        break;
                    }
                    buf = new unsigned char [9];
                    int numRecv = recv(socket,buf,9,0);
                    if(numRecv > 0){
                        handleMsg.unpacked_msg(msgRecv,buf,numRecv);
                        if(msgRecv.type_msg == RSP){
                            // continue chat
                            break;
                        }
                    }
                    else if(numRecv == 0){
                        close(socket);
                        std::cout<<"server is close!!!\n";
                        stop = 1;
                        break;
                    }
                    else if(numRecv == -1){
                        perror("recv: ");
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
bool isReconnect(){
    std::string ans;

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
            std::thread recvThread(recvMsg,bufrcv,socket);
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

        }

        reconnect = isReconnect();

    }
    return 0;
}
