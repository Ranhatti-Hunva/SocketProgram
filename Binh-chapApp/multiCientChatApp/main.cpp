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
#include <dirent.h>
#include <errno.h>
#include <cstddef>        // std::size_t
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <map>
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include "md5.h"
#include "threadpool.h"
#include <fstream>
#include <thread>
#include "msgqueue.h"
#include "csqueue.h"
//---------------------------------------------------------------------------------------
#define HOST "localhost"
#define PORT "8096"
#define TIME_OUT 10
#define MAX_FILE_TXT 1024
#define NUM_CLIENT 25
//------------variable check-------------------------------------------------------------

int stop = 0;
long int timeOut = 5000; //ms
int loginId = -1;

std::mutex mtex;
std::string name[NUM_CLIENT];
bool reconnectClient[NUM_CLIENT];
//-----struct timeout ---------------------------------------------------------
struct timeoutSend{
    long int time;
    msg_text msg;
    int msgId;
};

//------------thread receive msg from server---------------------------------------------
void recvMsg(unsigned char *buf,
             int sockfd,
             std::vector<timeoutSend>&timeoutQ,
             std::string name,
             msgQueue &msg_Q,
             std::mutex &mt,
             int &count,int &stopTr
             ){
    msg_text msgSend;
    fd_set listener;
    FD_ZERO(&listener);
    FD_SET(sockfd,&listener);
    while(stopTr!=1){
        if(select(sockfd+1,&listener, nullptr,nullptr,nullptr) == -1){
            perror("select");
        }
        if(FD_ISSET(sockfd,&listener)){
            buf = new unsigned char [3072];
            memset(buf,0,3072);

            int bytesRecv = recv(sockfd,buf,3072,0);
            if(bytesRecv >0){
                struct msg_text msg_get;
                struct msg_text msg_rsp;
                HandleMsg handleMsg;
                //std::cout<<"bytes recv "<<bytesRecv<<"\n";

                std::vector<unsigned char> buffer;
                buffer.insert(buffer.end(),&buf[0],&buf[bytesRecv]);
                while(buffer.size()>0){
                    bool is_success = handleMsg.unpacked_msg(msg_get,buffer);
                    if(!is_success && (buffer.size() > 0)){
                        break;
                    }
                    else{
                        if(is_success && msg_get.type_msg == RSP && msg_get.ID == loginId){
                        }
                        if(is_success && msg_get.type_msg != RSP){
                            uint8_t buffer[9];
                            msg_rsp.ID = msg_get.ID;
                            msg_rsp.type_msg = RSP;
                            msg_rsp.msg.clear();
                            handleMsg.packed_msg(msg_rsp,buffer);
                            msg_Q.pushQ(msg_rsp,mtex);
                        }
                        if(msg_get.type_msg == RSP){
                            //std::cout << "id rps> " << msg_get.ID << std::endl;
                            std::this_thread::sleep_for(std::chrono::milliseconds(1));
                            mt.lock();
                            std::vector<timeoutSend>::iterator it;
                            // tim va xoa msg trong Q timeout khi nhan respond
                            if(!timeoutQ.empty()){
                                it = std::find_if(timeoutQ.begin(),timeoutQ.end(),
                                                  [=] (timeoutSend const& f) {
                                    return (f.msgId == msg_get.ID);
                                });
                                //tim thay va xoa
                                if (it != timeoutQ.end()){
                                    timeoutQ.erase(it);
                                }
                            }
                            mt.unlock();
                        }
                        if(msg_get.type_msg == MSG){
                            std::cout <<"==> Nguoi nhan "<<name<< "> " << msg_get.msg << std::endl;

                            //ping pong msg
                            std::size_t found = msg_get.msg.find_first_of(">");
                            if(found != std::string::npos){
                                std::string data = msg_get.msg.substr(found+1,msg_get.msg.size());
                                //mt.lock();
                                //std::this_thread::sleep_for(std::chrono::milliseconds(1));
                                if(count == 0){
                                    msgSend.type_msg = MSG;
                                    msg_get.msg[found] = '/';
                                    msgSend.msg.assign(msg_get.msg);
                                    msg_Q.pushQ(msgSend,mtex);
                                    //msgQ.push(msgSend);

                                }else{
                                    count--;
                                    if(count < 0){
                                        count = 0;
                                    }
                                }
                                //mt.unlock();
                            }
                        }
                    }
                }
            }
            if(bytesRecv == 0){
                std::cout << "> server is close \n";
                close(sockfd);
                stop = 1;
                break;
            }
            if ((bytesRecv == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK))){
                perror("=> Message is not gotten complete !!");
            }
            if(stop != 0){
                break;
            }
            delete[] buf;
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
//------------Read All Bytes in file ----------------------------------------------------
std::string ReadAllBytes(const char * filename, int * read)
{
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    int length = pos;

    char *pChars = new char[pos];
    ifs.seekg(0, std::ios::beg);
    ifs.read(pChars, length);
    ifs.close();
    *read = length;
    std::string strReturn = std::string(pChars,0,length);
    delete [] pChars;
    return strReturn;
}

//------------struct file node-----------------------------------------------------------
struct fileNode{
    std::string filePath;
    std::string hashVa;
};

//------------get file name--------------------------------------------------------------
int getdir (std::string dir, std::vector<std::string> &files)
{
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == nullptr) {
        std::cout << "Error(" << errno << ") opening " << dir << std::endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != nullptr) {
        std::size_t found = std::string(dirp->d_name).find_last_of(".");
        std::string txtExtension = std::string(dirp->d_name)
                .substr(found+1,dirp->d_namlen);
        if(strcmp(txtExtension.c_str(),"txt") == 0){
            files.push_back(std::string(dirp->d_name));
        }
    }
    closedir(dp);
    return 0;
}

//----send msg in file ------------------------------------------------------------------
void sendMsgInFile(std::string name,
                   std::string filePath,
                   msgQueue &msg_Q,
                   int &count){
    std::ifstream ifs(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    ifs.seekg(0, std::ios::beg);

    int block = 1024; // 1 KB
    int numberBlock = (pos/block) + 1;

    for(int i = 0; i < numberBlock; i++){

        char pChars[block] ;
        memset(pChars,0,block);

        ifs.read(pChars,block);
        //std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";
        msg_text msgSend;
        msgSend.type_msg = MSG;
        msgSend.msg.clear();
        msgSend.msg.assign(name + "/" +pChars,0,name.size() + 1 + sizeof (pChars));

        std::cout<<"==> Size  "<< msgSend.msg.size() <<"\n";
        //msgQ.push(msgSend);
        msg_Q.pushQ(msgSend,mtex);
        count++;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    ifs.close();

}
//----compare hash  ----------------------------------------------------------------
//return true if hash value equal

bool compareHashvalue(std::vector<fileNode>&fileList,std::string fileName){

    for(uint i = 0; i < fileList.size() ; i++){
        if(strcmp(fileList[i].filePath.c_str(),fileName.c_str()) == 0){
            int len;
            std::string str = ReadAllBytes(fileName.c_str(),&len);
            if(strcmp(fileList[i].hashVa.c_str(),md5(str).c_str()) == 0){
                return true;
            }
            fileList[i].hashVa.clear();
            fileList[i].hashVa.assign(md5(str));
            return false;
        }
    }

    int len;
    std::string str = ReadAllBytes(fileName.c_str(),&len);
    fileNode node;
    node.hashVa.assign(md5(str));
    node.filePath.assign(fileName);
    fileList.push_back(node);
    return false;
}

//----thread send file ------------------------------------------------------------------

void sendFileThread(std::string name,
                    msgQueue &msg_Q,
                    bool &stopFile,int &count){

    //lay list file name
    //ghep vs ten username r send
    //kiem tra thay doi
    //tiep tuc gui
    const char * folderPath = "../readFile";
    std::string dir;
    dir.assign(folderPath);
    std::vector<std::string> files = std::vector<std::string>();
    getdir(dir,files);
    std::vector<fileNode>fileList;
    fileList.clear();

    for (uint i = 0; i < files.size(); i++) {
        std::string fullName;
        fullName.assign("../readFile/"+files[i]);
        int len;
        std::string plaintext = ReadAllBytes(fullName.c_str(),&len);
        fileNode node;
        node.hashVa = md5(plaintext);
        node.filePath.assign(fullName);
        fileList.push_back(node);
        //gui moi
        sendMsgInFile(name,fullName,std::ref(msg_Q),count);
    }

    int kQ = kqueue();
    int dirFd = open(folderPath,O_RDONLY);
    struct kevent dirEvent;
    EV_SET(&dirEvent,dirFd, EVFILT_VNODE,EV_ADD|EV_CLEAR|EV_ENABLE,NOTE_WRITE,0,(void *)folderPath);
    kevent(kQ, &dirEvent, 1, nullptr, 0, nullptr);

    struct timespec time_delay;
    time_delay.tv_sec = 1;
    time_delay.tv_nsec = 0;

    while(!stopFile){
        struct kevent change;
        memset(&change,0,sizeof(change));
        if(kevent(kQ,nullptr,0,&change, 1, &time_delay) == -1){
            stopFile = true;
            break;
        }
        else if(change.udata != nullptr){
            std::vector <std::string> fileChange;
            getdir(dir,fileChange);
            for(uint i = 0; i <fileChange.size();i++){
                std::string fullName;
                fullName.assign("../readFile/"+fileChange[i]);
                if(compareHashvalue(fileList,fullName) == false){
                    // gui moi
                    sendMsgInFile(name,fullName,std::ref(msg_Q),count);
                }
            }
        }
    }
}

//----thread nhan tu console-------------------------------------------------------------

void cinFromConsole(csQueue &cs_Q,
                    std::string (name)[]){

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

            if(strcmp(userInput.c_str(),"#") == 0){
                stop = 1;
                break;
            }
            else if(strcmp(userInput.c_str(),"*") == 0){

                std::this_thread::sleep_for(std::chrono::seconds(3));

                std::ifstream ifs("../readFile/a.txt", std::ios::binary|std::ios::ate);
                std::ifstream::pos_type pos = ifs.tellg();
                ifs.seekg(0, std::ios::beg);
                int block = 256; // 1/8 KB
                int numberBlock = (pos/block) + 1;
                std::string arrFile[numberBlock];
                for(int i = 0; i < numberBlock; i++){
                    char pChars[block] ;
                    arrFile[i] = new char[block];
                    memset(pChars,0,block);
                    arrFile[i].clear();
                    ifs.read(pChars,block);
                    arrFile[i].assign(pChars,0,sizeof (pChars));
                }
                ifs.close();
                userInput.clear();

                std::this_thread::sleep_for(std::chrono::seconds(1));
                for(int i = 0; i < numberBlock; i++){
                    for(int j = 0; j < NUM_CLIENT; j++){
                        //mt.lock();
                        nodeConsole node;
                        node.name.assign("B"+std::to_string(j));
                        node.data.assign("V"+std::to_string(j)+"/"+arrFile[i]);
                        cs_Q.pushQ(node);
                        //csQ.push(node);
                        //mt.unlock();
                    }
                    //std::cout<<"size ======><====== "<<cs_Q.size()<<"\n";
                }

                //                                std::this_thread::sleep_for(std::chrono::seconds(5));

                //                                std::ifstream ifs("../readFile/a.txt", std::ios::binary|std::ios::ate);
                //                                std::ifstream::pos_type pos = ifs.tellg();
                //                                ifs.seekg(0, std::ios::beg);
                //                                int block = 128; // 1/8 KB
                //                                int numberBlock = (pos/block) + 1;

                //                                for(int i = 0; i < numberBlock; i++){
                //                                    //mt.lock();
                //                                    char pChars[block] ;
                //                                    memset(pChars,0,block);
                //                                    ifs.read(pChars,block);
                //                                    //mt.unlock();
                //                                    //std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";

                //                                    for(int j = 0; j < NUM_CLIENT; j++){
                //                                        mt.lock();
                //                                        nodeConsole node;
                //                                        node.name.assign("B"+std::to_string(j));
                //                                        node.data.assign("V"+std::to_string(j)+"/"+pChars);
                //                                        csQ.push(node);
                //                                        mt.unlock();
                //                                        //std::cout<<"==>  i "<<i<<"\n";
                //                                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                //                                    }
                //                                    std::cout<<"=======><====== "<<i<<" "<<csQ.size()<<"\n";

                //                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                //                                }
                //                                ifs.close();
                //                                userInput.clear();

            }
            else if(userInput.size() >0){
                nodeConsole node;
                std::size_t found = userInput.find_first_of("/");
                if(found != std::string::npos){
                    node.name = userInput.substr(0,found);
                    node.data = userInput.substr(found+1,userInput.size());

                    for(int i = 0 ; i < NUM_CLIENT ; i++){
                        if(strcmp(node.name.c_str(),name[i].c_str()) == 0){
                            //std::cout<<"==> data "<<node.data<<"\n";
                            if(strcmp(node.data.c_str(),"rc") == 0){
                                reconnectClient[i] = true;
                                break;
                            }
                            //mt.lock();
                            cs_Q.pushQ(node);
                            //csQ.push(node);
                            //mt.unlock();
                            break;
                        }
                    }
                }else{
                    std::cout<<"wrong format!!/n";
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
//------------
void inputForThread(int socket,
                    msgQueue &msg_Q,
                    thread_pool &pool,
                    csQueue &cs_Q,
                    std::string name,
                    int &count,int &stopTr){

    std::string folderPath = "../readFile";
    msg_text msgSend;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    bool stopFile = false;
    nodeConsole node;
    while(stopTr!=1){
        usleep(1000);
        while(!cs_Q.isEmpty()){
            node.name.assign(cs_Q.frontQ().name);
            node.data.assign(cs_Q.frontQ().data);
            if(strcmp(node.name.c_str(),name.c_str()) == 0){
                if(strcmp(node.data.c_str(),"#") == 0){
                    close(socket);
                    stopTr = 1;
                    cs_Q.popQ();
                    break;
                }
                else if(strcmp(node.data.substr(0,4).c_str(),"file") == 0){
                    stopFile = false;
                    std::string username = node.data.substr(5,node.data.size());

                    if(!username.empty()){
                        //add thread
                        pool.enqueue([&,username]{
                            sendFileThread(username,std::ref(msg_Q),stopFile,count);
                        });
                    }
                }
                else if(strcmp(node.data.c_str(),"un_file/") == 0){
                    stopFile = true;
                }
                else if(node.data.size() >0){
                    //mt.lock();
                    msgSend.type_msg = MSG;
                    msgSend.msg.assign(node.data);
                    msg_Q.pushQ(msgSend,mtex);
                    count++;
                }
                cs_Q.popQ();
            }
        }
    }
}
//----------------------------------------------------------------------------------------
int sendall(int socket,unsigned char *buf, int &len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(socket, buf+total,bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
}

//------------thread send msg to server--------------------------------------------------
void sendMsg(int socket,
             msgQueue &msg_Q,
             std::vector<timeoutSend>&timeoutQ,
             std::mutex &mt,int &stopTr){
    HandleMsg handleMsg;
    struct timeval tp;
    while(stopTr!=1){
        if(!msg_Q.isEmpty(mt)){
            //mt.lock();

            int buferSize = msg_Q.frontQ(mt).msg.length()+9;

            unsigned char *buf = new unsigned char [buferSize];
            memset(buf,0,buferSize);
            //std::unique_ptr <unsigned char> buf (new unsigned char [buferSize]);
            msg_text msg;
            msg.ID = msg_Q.frontQ(mt).ID;
            msg.msg = msg_Q.frontQ(mt).msg;
            msg.type_msg = msg_Q.frontQ(mt).type_msg;
            handleMsg.packed_msg(msg,buf);

            int numSend = sendall(socket,buf,buferSize);
            if( numSend == 0){
                timeoutSend node;
                node.msg = msg_Q.frontQ(mt);
                node.msgId = msg_Q.frontQ(mt).ID;
                gettimeofday(&tp, nullptr);
                node.time = tp.tv_sec*1000 +tp.tv_usec/1000;
                timeoutQ.push_back(node);
                msg_Q.popQ(mt);
            }
            else{
                perror("send: ");
            }
            delete []buf;
        }
        else{
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
}
//-----send ping-------------------------------------------------------------------------
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

//------------thread timeout msg --------------------------------------------------------
void timeoutThread(int socket,
                   std::vector<timeoutSend>&timeoutQ,
                   msgQueue &msg_Q){
    struct timeval tp;
    HandleMsg handleMsg;
    std::unique_ptr <unsigned char> buf (new unsigned char [9]);
    //unsigned char *buf;
    msg_text msgRecv;
    while(stop != 1){
        usleep(1000);
        if(!timeoutQ.empty()){
            gettimeofday(&tp, nullptr);
            if((tp.tv_sec * 1000 + tp.tv_usec / 1000) - timeoutQ.front().time> timeOut){
                std::cout<<"timeout!!!\n";
                gettimeofday(&tp, nullptr);
                long int timePing = tp.tv_sec * 1000 + tp.tv_usec / 1000;
                //ping and wait rsp from server
                sendPing(socket);
                while(1){
                    gettimeofday(&tp, nullptr);
                    if(tp.tv_sec * 1000 + tp.tv_usec / 1000 - timePing>timeOut){
                        std::cout<<"can't connect to server\n";
                        close(socket);
                        stop = 1;
                        break;
                    }
                    //buf = new unsigned char [9];
                    int numRecv = recv(socket,buf.get(),9,0);
                    if(numRecv > 0){
                        int flagbreak = false;
                        std::vector<unsigned char> buffer;
                        buffer.insert(buffer.end(),&buf.get()[0],&buf.get()[numRecv]);
                        while(buffer.size()>0){
                            //std::cout<<"co ping\n";
                            bool is_success = handleMsg.unpacked_msg(msgRecv,buffer);
                            if(!is_success){
                                break;
                            }
                            else{
                                if(msgRecv.type_msg == RSP){
                                    // continue chat resend msg erease msg in timeoutQ
                                    //resend msg;
                                    //msgQ.push(timeoutQ.front().msg);
                                    msg_Q.pushQ(timeoutQ.front().msg,mtex);
                                    timeoutQ.erase(timeoutQ.begin());
                                    std::cout<<"resend and continue chat\n";
                                    flagbreak = true;
                                    break;
                                }
                            }
                        }
                        if(flagbreak){
                            break;
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
            }
        }
    }
}


//------------reconnect function---------------------------------------------------------
bool isReconnect(std::vector<timeoutSend>&timeoutQ){
    std::string ans;
    timeval now;

    while(!timeoutQ.empty()){
        gettimeofday(&now, nullptr);
        std::cout <<"content " <<timeoutQ.back().msg.msg<<"\n";
        std::cout <<"id " <<timeoutQ.back().msgId<<"\n";
        std::cout <<"time " <<timeoutQ.back().time - (now.tv_sec*1000+now.tv_usec/1000)<<"\n";
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
csQueue csListQ;
//------------main-----------------------------------------------------------------------
int main()
{

    //int ping_pong[NUM_CLIENT];
    std::string nameCin;
    getline(std::cin,nameCin);

    thread_pool threads_master(NUM_CLIENT+1);

    for(int i = 0; i< NUM_CLIENT; i++)
    {
        name[i] = nameCin + std::to_string(i);
        reconnectClient[i] = true;
    }


    //std::thread cinConsoleThread(cinFromConsole,ref(csList),ref(mtex),ref(name));
    threads_master.enqueue([&]{
        cinFromConsole(csListQ,name);
    });
    while(stop != 1){
        for(int i = 0; i< NUM_CLIENT; i++){
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(reconnectClient[i] == true){
                //std::cout << "==> co vao ano " << reconnectClient[i] << "\n";
                threads_master.enqueue([=](){
                    struct msg_text msgSend;
                    msgSend.type_msg = SGI;
                    ClientChat client;
                    unsigned char * bufrcv; // buf for recv data
                    std::string userInput;
                    std::vector <timeoutSend> timeoutList;
                    thread_pool pool(5);
                    std:: queue <msg_text> qSend;
                    msgQueue msgQ;
                    int count = 0;
                    //bool connect = true;
                    int stopTr = 0;
                    std::string hashStr;
                    //while(connectttt && (stop != 1 || stopTr == 0)){

                    msgSend.msg.assign(name[i]);
                    unsigned char buffer[msgSend.msg.length()+9];
                    HandleMsg handleMsg;
                    handleMsg.packed_msg(msgSend,buffer);

                    //connect to server
                    int socket = client.timeoutConnect(HOST,PORT,TIME_OUT);

                    //connect success
                    if(socket > 0){
                        //send name user to server for login
                        int num = send(socket,buffer,sizeof(buffer),0);
                        loginId = msgSend.ID;
                        mtex.lock();
                        std::cout<<"client "<<name[i]<<" connect to server\n";
                        mtex.unlock();
                        //socket non-blocking mode
                        long arg = fcntl(socket, F_GETFL, NULL);
                        arg |= O_NONBLOCK;
                        fcntl(socket, F_SETFL, arg);
                        // main thread for send msg

                        // press # for logout

                        pool.enqueue([=,&msgQ,&pool,&count,&stopTr]{
                            inputForThread(socket,msgQ,pool,csListQ,name[i],count,stopTr);
                        });
                        pool.enqueue([&]{
                            recvMsg(bufrcv,socket,ref(timeoutList),name[i],std::ref(msgQ),mtex,count,stopTr);
                        });
                        pool.enqueue([&]{
                            sendMsg(socket,std::ref(msgQ),ref(timeoutList),ref(mtex),std::ref(stopTr));
                        });

                    }
                });
                reconnectClient[i] = false;
            }
        }
        if(stop == 1) break;
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
    return 0;
}
