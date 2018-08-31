#include "clientchat.h"
#include "handlemsg.h"
#include <sys/time.h>
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
    while(1){

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
        // mtx.lock();
        int bytesRecv = recv(sockfd,buf,2048,0);
        // mtx.unlock();
        //mo goi
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
                send(sockfd,buffer,10,0);
            }

            if(msg_get.type_msg == RSP){
                timeRSP = 0;
                ms = 0;
                // std::cout << "id rps> " << msg_get.ID << std::endl;
            }
            if(msg_get.msg.length() > 0)
                std::cout << "> " << msg_get.msg << std::endl;
        }
        if(bytesRecv == 0){
            std::cout << "> server is close \n";
            close(sockfd);
            exit(1);
        }
        if(stop != 0){
            break;
        }
        delete[] buf;
    }
}
//------------reconnect function---------------------------------------------------
bool isReconnect(){
    std::string ans;
    std::cout<<"co vao 123\n";
    //return true;
    while(1){
        std::cout << "Do you want reconnect to server ? (Y/N)\n";
        //std::cin.ignore();
        getline(std::cin,ans);
        if(ans.compare("Y") == 0){
            return true;
        }
        else if(ans.compare("N") == 0){
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
    //    std::cout << "IP address: ";

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
                if(numRecv >0){
                    //std::cout<<"strcmp success "<<strcmp(statusBuf,"success")<<"\n";
                    //std::cout<<"msg recv "<<std::string(statusBuf,0,10)<<"\n";
                    if(strcmp(statusBuf,"success") == 0){
                        std::cout << "Login success\n";
                        std::cout << "Start chat now" <<"\n";
                        std::cout  << "press '#' to exit\n";
                        usleep(1000);
                        break;
                    }
                    if(strcmp(statusBuf,"existed") == 0){
                        std::cout << "Username is existed please use another name\n>";

                        //std::cin.ignore();
                        getline(std::cin,name);
                        msgSend.msg.clear();
                        msgSend.msg.assign(name);
                        unsigned char nameResend[msgSend.msg.length()+9];
                        handleMsg.packed_msg(msgSend,nameResend);
                        send(socket,nameResend,sizeof(nameResend),0);
                    }
                    if(strcmp(statusBuf,"full") == 0){
                        std::cout << "server is full !!! can't login server\n";
                        close(socket);
                        return 1;
                    }
                }
            }

            //socket non-blocking mode
            fcntl(socket, F_SETFL, O_NONBLOCK);


            //thread receive msg
            std::thread recvThread(recvMsg,bufrcv,socket);

            // main thread for send msg
            // press # for logout
            while(1){
                std::cout  << ">";
                getline(std::cin,userInput);
                if(strcmp(userInput.c_str(),"#") == 0){
                    send(socket,"",0,0);
                    close(socket);
                    //end_connection = true;
                    stop = 1;
                    //std::terminate();
                    //break;
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
                    ms = (tp.tv_sec * 1000 + tp.tv_usec / 1000);
                    mtx.unlock();

                    if(sendResult == -1){
                        perror("send");
                        break;
                    }
                }
            }
            //stop thread recv before reconnect
            recvThread.join();
        }
        reconnect = isReconnect();
    }
    return 0;
}
