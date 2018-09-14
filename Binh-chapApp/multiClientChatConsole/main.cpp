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
//---------------------------------------------------------------------------------------
#define HOST "10.42.0.126"
#define PORT "8096"
#define TIME_OUT 10
#define MAX_FILE_TXT 1024
#define NUM_CLIENT 15
//------------variable check-------------------------------------------------------------
std::mutex mtx;
int stop = 0;
long int timeOut = 5000; //ms
//std::chrono::milliseconds ms;
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
             std::string &hashStr,
             std::queue<msg_text> &msgQ,
             std::mutex &mt,
             int &count,int &stopTr
             ){
    msg_text msgSend;
    while(stopTr!=1){

        buf = new unsigned char [4096];
        memset(buf,0,4096);

        int bytesRecv = recv(sockfd,buf,4096,0);
        if(bytesRecv >0){
            //printf("vao \n");
            struct msg_text msg_get;
            struct msg_text msg_rsp;
            HandleMsg handleMsg;
            //std::cout<<"bytes recv "<<bytesRecv<<"\n";

            std::vector<unsigned char> buffer;
            buffer.insert(buffer.end(),&buf[0],&buf[bytesRecv]);

            //bool is_success = handleMsg.unpacked_msg(msg_get, buf, bytesRecv);
            while(buffer.size()>0){
                bool is_success = handleMsg.unpacked_msg(msg_get,buffer);
                if(!is_success && (buffer.size() > 0)){

                    break;
                }
                else{


                    if(is_success && msg_get.type_msg == RSP && msg_get.ID == loginId){

                        //                        std::cout << "Login success\n";
                        //                        std::cout << "Start chat now" <<"\n";
                        //                        std::cout  << "press '#' to exit\n";

                    }

                    if(is_success && msg_get.type_msg != RSP){
                        unsigned char buffer[10];
                        msg_rsp.ID = msg_get.ID;
                        msg_rsp.type_msg = RSP;
                        handleMsg.packed_msg(msg_rsp,buffer);
                        //send(sockfd,buffer,10,0);
                        mt.lock();
                        msgQ.push(msg_rsp);
                        mt.unlock();
                        //std::cout << "id rps> " << msg_rsp.ID << std::endl;
                        //std::vector<timeoutSend>::iterator it;
                    }
                    if(msg_get.type_msg == RSP){
                        //std::cout << "id rps> " << msg_get.ID << std::endl;

                        usleep(1000);

                        mt.lock();
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
                                //std::cout<<"xoa "<<it.base()->msgId<<"\n";
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
                            mt.lock();
                            usleep(1000);
                            if(count == 0){

                                msgSend.type_msg = MSG;
                                msg_get.msg[found] = '/';
                                //std::cout<<"msg "<<msg_get.msg<<"\n";
                                msgSend.msg.assign(msg_get.msg);
                                msgQ.push(msgSend);

                            }else{
                                count--;
                                if(count < 0){
                                    count = 0;
                                }
                            }
                            mt.unlock();
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
            //exit(1);
        }
        if(stop != 0){
            break;
        }
        delete[] buf;
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
    //int id;
    std::string filePath;
    //bool isSend;
    //time_t timeModify;
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
                   std::queue<msg_text>&msgQ,int &count,std::mutex &mt){

    std::ifstream ifs(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    int block = 1024; // 1 KB

    int numberBlock = (pos/block) + 1;
    //std::cout<<"pos number  "<<pos<<"\n";
    //std::cout<<"block number  "<<numberBlock<<"\n";
    for(int i = 0; i < numberBlock; i++){
        mt.lock();
        char pChars[block] ;
        memset(pChars,0,block);

        ifs.read(pChars,block);
        //std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";

        msg_text msgSend;
        msgSend.type_msg = MSG;
        msgSend.msg.clear();
        msgSend.msg.assign(name + "/" +pChars);

        std::cout<<"==> Size  "<< msgSend.msg.size() <<"\n";
        msgQ.push(msgSend);
        //        HandleMsg handleMsg;
        //        uint8_t a[msgSend.msg.size()+10];
        //        handleMsg.packed_msg(msgSend,a);
        //        send(socket,a,sizeof(a),0);
        count++;
        usleep(10000);


        //delete []pChars;
        mt.unlock();
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

            //            std::cout<<"name 1 "<<fileName.c_str()<<"\n";
            //            std::cout<<"name 2 "<<fileList[i].filePath<<"\n";
            //            std::cout<<"hash 1 "<<fileList[i].hashVa<<"\n";
            //            std::cout<<"hash 2 "<<md5(str)<<"\n";

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
                    std::queue<msg_text>&msgQ,
                    bool &stopFile,int &count,std::mutex &mt){

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

    //std::cout<<"co vao 1 "<<name<<"\n";

    for (uint i = 0; i < files.size(); i++) {
        //std::cout<<"co vao 2 "<<files[i]<<"\n";
        std::string fullName;
        fullName.assign("../readFile/"+files[i]);
        //std::cout<<fullName<<" \n";

        int len;
        std::string plaintext = ReadAllBytes(fullName.c_str(),&len);

        fileNode node;
        node.hashVa = md5(plaintext);
        node.filePath.assign(fullName);

        fileList.push_back(node);
        //gui moi
        //mt.lock();
        sendMsgInFile(name,fullName,ref(msgQ),count,mt);
        //mt.unlock();
    }

    int kQ = kqueue();
    int dirFd = open(folderPath,O_RDONLY);
    //std::cout<<" dir fd "<<dirFd<<"\n";
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

            //std::cout<<"co vao day\n";
            std::vector <std::string> fileChange;

            getdir(dir,fileChange);

            for(uint i = 0; i <fileChange.size();i++){
                std::string fullName;
                fullName.assign("../readFile/"+fileChange[i]);
                if(compareHashvalue(fileList,fullName) == false){
                    // gui moi
                    //std::cout<<"co vao day\n";

                    sendMsgInFile(name,fullName,ref(msgQ),count,mt);

                }
            }


        }
    }

}
//
struct nodeConsole{
    std::string name;
    std::string data;

};

//----thread nhan tu console-------------------------------------------------------------

void cinFromConsole(std::queue<nodeConsole>&csQ,std::mutex &mt,
                    std::string (name)[]){

    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    //bool stopFile = false;


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

                sleep(2);

                std::ifstream ifs("../readFile/a.txt", std::ios::binary|std::ios::ate);
                std::ifstream::pos_type pos = ifs.tellg();
                ifs.seekg(0, std::ios::beg);
                int block = 128; // 1/8 KB

                int numberBlock = (pos/block) + 1;

                for(int i = 0; i < numberBlock; i++){
                    mt.lock();
                    char pChars[block] ;
                    memset(pChars,0,block);
                    ifs.read(pChars,block);
                    mt.unlock();
                    //std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";

                    for(int i = 0; i < NUM_CLIENT; i++){
                        mt.lock();
                        nodeConsole node;
                        node.name.assign("B"+std::to_string(i));
                        node.data.assign("V"+std::to_string(i)+"/"+pChars);
                        csQ.push(node);
                        usleep(1000);
                        mt.unlock();
                    }

                    usleep(10000);
                    //                    if(sizeof(pChars) != block){
                    //                        break;
                    //                    }
                    //mt.unlock();
                }
                ifs.close();
                userInput.clear();

            }
            else if(userInput.size() >0){
                nodeConsole node;
                std::size_t found = userInput.find_first_of("/");
                if(found != std::string::npos){
                    node.name = userInput.substr(0,found);
                    node.data = userInput.substr(found+1,userInput.size());
                    //std::cout<<node.data<<'\n';
                    for(int i = 0 ; i < NUM_CLIENT ; i++){

                        if(strcmp(node.name.c_str(),name[i].c_str()) == 0){
                            //std::cout<<"==> data "<<node.data<<"\n";

                            if(strcmp(node.data.c_str(),"rc") == 0){
                                //std::cout<<"co vao\n";
                                reconnectClient[i] = true;
                                break;
                            }
                            mt.lock();
                            //std::cout<<"node co vao " << node.data << "\n";
                            csQ.push(node);
                            mt.unlock();

                            break;
                        }
                    }

                }else{
                    std::cout<<"wrong format!!/n";
                }
            }
        }
    }
}
//------------
void inputForThread(int socket,
                    std::queue<msg_text>&msgQ,
                    thread_pool &pool,
                    std::queue<nodeConsole>&csQ,
                    std::mutex &mt,
                    std::string name,
                    std::string &hashStr,int &count,int &stopTr){
    //int socket;std::queue<msg_text>msgQ;

    std::string folderPath = "../readFile";
    msg_text msgSend;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    bool stopFile = false;
    //std::queue<nodeConsole> csQThread;

    while(stopTr!=1){
        //csQThread = csQ;
        if(!csQ.empty()){
            nodeConsole node;
            node.name.assign(csQ.front().name);
            node.data.assign(csQ.front().data);
            if(strcmp(node.name.c_str(),name.c_str()) == 0){
                if(strcmp(node.data.c_str(),"#") == 0){
                    mt.lock();
                    close(socket);
                    stopTr = 1;
                    csQ.pop();
                    mt.unlock();

                    break;
                }
                else if(strcmp(node.data.substr(0,4).c_str(),"file") == 0){
                    stopFile = false;
                    std::string username = node.data.substr(5,node.data.size());

                    if(!username.empty()){
                        //add thread
                        //std::cout<<"usr name "<<username<<"\n";
                        pool.enqueue([&,username]{
                            sendFileThread(username,ref(msgQ),stopFile,count,mt);
                        });
                    }

                }
                else if(strcmp(node.data.c_str(),"un_file/") == 0){
                    stopFile = true;
                }
                else if(node.data.size() >0){
                    mt.lock();
                    msgSend.type_msg = MSG;
                    msgSend.msg.assign(node.data);


                    //                    std::size_t found = node.data.find_first_of("/");
                    //                    if(found != std::string::npos){
                    //                        hashStr.assign(md5(node.data.substr(found+1,node.data.size())));
                    //                        //std::cout<<"hash data "<< hashStr<<"\n";
                    //                    }

                    msgQ.push(msgSend);
                    count++;
                    mt.unlock();

                }
                mt.lock();
                csQ.pop();
                mt.unlock();
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
        n = send(socket, buf+total, std::min(bytesleft,len), 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

//------------thread send msg to server--------------------------------------------------
void sendMsg(int socket,std::queue<msg_text>&msgQ,
             std::vector<timeoutSend>&timeoutQ,std::mutex &mt,int &stopTr){
    HandleMsg handleMsg;
    struct timeval tp;
    while(stopTr!=1){
        if(!msgQ.empty()){
            mt.lock();
            int buferSize = msgQ.front().msg.length()+9;

            unsigned char *buf = new unsigned char [buferSize];
            memset(buf,0,buferSize);
            //std::unique_ptr <unsigned char> buf (new unsigned char [buferSize]);
            handleMsg.packed_msg(msgQ.front(),buf);

            //            std::cout<<"\nlbi  --- "<<buferSize<<" "<< msgQ.size()<<"\n";
            //            for(int i = 0; i < buferSize;i++){

            //                printf("%c",msgQ.front().msg.c_str()[i]);
            //            }
            //std::cout<<"\n";

            //std::cout <<"==> numSend 2 "<< buferSize <<"\n";
            int numSend = sendall(socket,buf,buferSize);
            if( numSend == 0){

                //std::cout<<"==> numSend " << buferSize <<"\n";
                timeoutSend node;
                node.msg = msgQ.front();
                node.msgId = msgQ.front().ID;
                gettimeofday(&tp, nullptr);
                node.time = tp.tv_sec*1000 +tp.tv_usec/1000;
                //std::cout<<"send id "<<node.msgId<<"\n";

                timeoutQ.push_back(node);
                msgQ.pop();
                usleep(2000);

            }
            else{
                perror("send: ");
            }
            mt.unlock();
            delete []buf;

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
                //usleep(100000);
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
                                    msgQ.push(timeoutQ.front().msg);

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
                mtx.unlock();
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
    }
}
std::queue<nodeConsole> csList;
//------------main-----------------------------------------------------------------------
int main()
{

    //int ping_pong[NUM_CLIENT];


    thread_pool threads_master(NUM_CLIENT+1);

    for(int i = 0; i< NUM_CLIENT; i++)
    {
        name[i] = "B" + std::to_string(i);
        reconnectClient[i] = true;
    }

    //std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend),ref(pool));
    std::thread cinConsoleThread(cinFromConsole,ref(csList),ref(mtex),ref(name));
    //    threads_master.enqueue([&]{
    //        cinFromConsole(csList,mtex,name);
    //    });
    while(1){
        for(int i = 0; i< NUM_CLIENT; i++){
            usleep(50000);
            //sleep(2);
            //std::cout << "==> num licen " << reconnectClient[i] << "\n";
            if(reconnectClient[i] == true){
                //std::cout << "==> co vao ano " << reconnectClient[i] << "\n";
                threads_master.enqueue([=](){
                    //std::queue<nodeConsole> csList;
                    //std::mutex mtex;
                    struct msg_text msgSend;
                    msgSend.type_msg = SGI;
                    ClientChat client;
                    unsigned char * bufrcv; // buf for recv data
                    std::string userInput;
                    std::vector <timeoutSend> timeoutList;
                    thread_pool pool(5);
                    std:: queue <msg_text> qSend;
                    int count = 0;
                    bool connectttt = true;
                    int stopTr = 0;
                    std::string hashStr;
                    while(connectttt && (stop != 1 || stopTr == 0)){

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


                            std::cout<<"client "<<name[i]<<" connect to server\n";
                            //socket non-blocking mode
                            long arg = fcntl(socket, F_GETFL, NULL);
                            arg |= O_NONBLOCK;
                            fcntl(socket, F_SETFL, arg);
                            // main thread for send msg

                            // press # for logout
                            //std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend),ref(pool));
                            pool.enqueue([=,&qSend,&pool,&hashStr,&count,&stopTr]{
                                inputForThread(socket,qSend,pool,csList,mtex,name[i],hashStr,count,stopTr);
                            });

                            pool.enqueue([&]{
                                recvMsg(bufrcv,socket,ref(timeoutList),name[i],hashStr,qSend,mtex,count,stopTr);
                            });
                            //                            pool.enqueue([&]{
                            //                                timeoutThread(socket,ref(timeoutList),ref(qSend));
                            //                            });
                            //                            pool.enqueue([&]{
                            //                                sendMsg(socket,ref(qSend),ref(timeoutList),ref(mtex),stopTr);
                            //                            });
                            std::thread sendMsgThread(sendMsg,socket,ref(qSend),ref(timeoutList),ref(mtex),std::ref(stopTr));

                            sendMsgThread.join();
                        }

                        //reconnectClient[i] = isReconnect(ref(timeoutList));

                    }

                });
                reconnectClient[i] = false;

            }

        }
        if(stop == 1) break;

    }


    //    struct msg_text msgSend;
    //    msgSend.type_msg = SGI;
    //    ClientChat client;
    //    unsigned char * bufrcv; // buf for recv data
    //    std::string userInput;
    //    std::vector <timeoutSend> timeoutList;
    //    thread_pool pool(10);


    //    std:: queue <msg_text> qSend;
    //    bool reconnect = true;



    //    while(reconnect){
    //        stop = 0;
    //        std::string name ="";

    //        msgSend.msg.assign(name);

    //        unsigned char buffer[msgSend.msg.length()+9];

    //        HandleMsg handleMsg;
    //        handleMsg.packed_msg(msgSend,buffer);

    //        //connect to server
    //        int socket = client.timeoutConnect(HOST,PORT,TIME_OUT);

    //        //connect success
    //        if(socket > 0){
    //            //send name user to server for login
    //            int num = send(socket,buffer,sizeof(buffer),0);
    //            loginId = msgSend.ID;

    //            //socket non-blocking mode
    //            long arg = fcntl(socket, F_GETFL, NULL);
    //            arg |= O_NONBLOCK;
    //            fcntl(socket, F_SETFL, arg);
    //            // main thread for send msg

    //            // press # for logout
    //            //std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend),ref(pool));

    //            pool.enqueue([&]{
    //                cinFromConsole(socket,ref(qSend),pool);
    //            });
    //            pool.enqueue([&]{
    //                recvMsg(bufrcv,socket,ref(timeoutList));
    //            });
    //            pool.enqueue([&]{
    //                timeoutThread(socket,ref(timeoutList),ref(qSend));
    //            });
    //            std::thread sendMsgThread(sendMsg,socket,ref(qSend),ref(timeoutList));

    //            sendMsgThread.join();
    //        }

    //        reconnect = isReconnect(ref(timeoutList));

    //    }
    return 0;
}
