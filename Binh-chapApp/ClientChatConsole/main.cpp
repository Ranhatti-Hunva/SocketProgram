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
//---------------------------------------------------------------------------------------
#define HOST "localhost"
#define PORT "8096"
#define TIME_OUT 10
#define MAX_FILE_TXT 1024
//------------variable check-------------------------------------------------------------
std::mutex mtx;
int stop = 0;
long int timeOut = 5000; //ms
//std::chrono::milliseconds ms;
int loginId = -1;
//-----struct timeout ---------------------------------------------------------
struct timeoutSend{
    long int time;
    msg_text msg;
    int msgId;
};

//------------thread receive msg from server---------------------------------------------

void recvMsg(unsigned char *buf,int sockfd,std::vector<timeoutSend>&timeoutQ){
    long int timeRSP = 0;
    struct timeval tp;
    while(stop!=1){

        buf = new unsigned char [2048];
        memset(buf,0,2048);

        int bytesRecv = recv(sockfd,buf,2048,0);


        if(bytesRecv >0){
            //printf("vao \n");
            struct msg_text msg_get;
            struct msg_text msg_rsp;
            HandleMsg handleMsg;
            //std::cout<<"bytes recv "<<bytesRecv<<"\n";
            std::vector<unsigned char> buffer;
            buffer.insert(buffer.end(),&buf[0],&buf[bytesRecv]);

            //            std::cout<<"recv \n";
            //            for(int i = 0; i< bytesRecv;i++){
            //                std::cout<<(unsigned int)buf[i]<<" ";
            //            }

            //bool is_success = handleMsg.unpacked_msg(msg_get, buf, bytesRecv);
            while(buffer.size()>0){
                bool is_success = handleMsg.unpacked_msg(msg_get,buffer);
                if(!is_success && (buffer.size() > 0)){
//                    printf("vao \n");
                    for(int i = 0; i < buffer.size();i++){
                        printf(" %d ",buffer[i]);
                    }
                    break;
                }
                else{
                    //std::cout<<"\n";
                    // gui rsp to

                    //                    if(is_success){
                    //                        std::cout<<"id "<<msg_get.ID<<" type "<<(int)msg_get.type_msg
                    //                                <<" content "<<msg_get.msg<<"\n";
                    //                    }

                    if(is_success && msg_get.type_msg == RSP && msg_get.ID == loginId){

                        std::cout << "Login success\n";
                        std::cout << "Start chat now" <<"\n";
                        std::cout  << "press '#' to exit\n";

                    }

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

                        usleep(1000);

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
                                //std::cout<<"xoa "<<it.base()->msgId<<"\n";
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
//------------struct file node-----------------------------------------------------------
struct fileNode{
    int id;
    std::string filePath;
    bool isSend;
    time_t timeModify;
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
        files.push_back(std::string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

//------------add file to Q send---------------------------------------------------------
void addFileList(std::vector<fileNode>&fileList,std::string fileName){
    std::map<std::string,int> mapFile;
    std::map<std::string,int>::iterator it;

    for(int i = 0; i< MAX_FILE_TXT; i++){
        if(fileList[i].id != -1){
            mapFile.insert(std::pair<std::string,int>(fileList[i].filePath,fileList[i].id));
        }
    }
    it = mapFile.find(fileName);
    if (it != mapFile.end()){
        //check time modify
        int posFile = it->second;
        struct stat attr;
        stat(fileList[posFile].filePath.c_str(), &attr);
        if(difftime(attr.st_mtime,fileList[posFile].timeModify) != 0){
            fileList[posFile].isSend = false;
            fileList[posFile].timeModify = attr.st_mtime; //update time
        }
    }
    else{
        //add new
        int posFile = mapFile.size();
        fileList[posFile].filePath.assign(fileName);
        fileList[posFile].id = posFile;
        fileList[posFile].isSend = false;
        struct stat attr;
        stat(fileList[posFile].filePath.c_str(), &attr);
        fileList[posFile].timeModify = attr.st_mtime;
    }

}

//----thread nhan tu console-------------------------------------------------------------

void cinFromConsole(int socket,std::queue<msg_text>&msgQ,std::vector<fileNode>&fileList){
    //int socket;std::queue<msg_text>msgQ;

    std::string folderPath = "../readFile";
    msg_text msgSend;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;


    //std::to_string(42);
    //test 1000 msg/s
//    sleep(15);
//    for(int i = 0;i <1000; i++){
//        msgSend.type_msg = MSG;
//        msgSend.msg.assign("all/hello "+std::to_string(i));
//        mtx.lock();
//        msgQ.push(msgSend);
//        usleep(1000);//1ms
//        mtx.unlock();

//    }

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
            else if(strcmp(userInput.substr(0,4).c_str(),"file") == 0){
                std::string dir;
                dir.assign(folderPath);
                std::vector<std::string> files = std::vector<std::string>();
                getdir(dir,files);
                for (unsigned int i = 0;i < files.size();i++) {

                    std::size_t found = files[i].find_last_of(".");
                    std::string txtExtension = files[i].substr(found+1,files[i].size());
                    //cout<<a<<endl;
                    if(strcmp(txtExtension.c_str(),"txt") == 0){

                        std::string fullName = folderPath +'/'+files[i];

                        std::cout<<fullName<<"\n";

                        addFileList(fileList,fullName);
                    }
                }
                //doc file and send
                for(int i = 0; i < MAX_FILE_TXT; i++){
                    if(fileList[i].id != -1 && fileList[i].isSend == false){
                        std::cout << "content of file "
                                  << fileList[i].filePath <<" will be send to ";
                        std::cout << userInput.substr(5,userInput.size()) << "\n";
                        std::string username = userInput.substr(5,userInput.size());

                        std::ifstream ifs(fileList[i].filePath, std::ios::binary|std::ios::ate);
                        std::ifstream::pos_type pos = ifs.tellg();
                        ifs.seekg(0, std::ios::beg);
                        int block = 1024;

                        int numberBlock = (pos/block) + 1;
                        std::cout<<"pos number  "<<pos<<"\n";
                        std::cout<<"block number  "<<numberBlock<<"\n";
                        for(int i = 0; i < numberBlock; i++){
                            char *pChars = new char[block];
                            memset(pChars,0,block);

                            ifs.read(pChars,block);
                            std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";
                            msgSend.type_msg = MSG;
                            msgSend.msg.assign(username + "/" +pChars);
                            mtx.lock();
                            //usleep(10000);
                            msgQ.push(msgSend);
                            mtx.unlock();
                            delete []pChars;
                        }
                        ifs.close();

                        fileList[i].isSend = true;
                    }
                }



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


//------------thread send msg to server--------------------------------------------------
void sendMsg(int socket,std::queue<msg_text>&msgQ,std::vector<timeoutSend>&timeoutQ){
    HandleMsg handleMsg;
    struct timeval tp;
    while(stop!=1){
        if(!msgQ.empty()){
            int buferSize = msgQ.front().msg.length()+9;

            unsigned char *buf = new unsigned char [buferSize];
            //std::unique_ptr <unsigned char> buf (new unsigned char [buferSize]);
            handleMsg.packed_msg(msgQ.front(),buf);

            //            std::cout<<"send buf\n";
            //            for(int i = 0; i< buferSize;i++){
            //                std::cout<<(unsigned int)buf.get()[i]<<" ";
            //            }
            //            std::cout<<"\n";

            if(send(socket,buf,buferSize,0) > 0){
                mtx.lock();
                timeoutSend node;
                node.msg = msgQ.front();
                node.msgId = msgQ.front().ID;
                gettimeofday(&tp, nullptr);
                node.time = tp.tv_sec*1000 +tp.tv_usec/1000;
                //std::cout<<"send id "<<node.msgId<<"\n";
                usleep(2000);
                timeoutQ.push_back(node);

                msgQ.pop();
                mtx.unlock();
            }
            else{
                perror("send: ");
            }
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

//------------main-----------------------------------------------------------------------
int main()
{

    struct msg_text msgSend;
    msgSend.type_msg = SGI;
    ClientChat client;
    unsigned char * bufrcv; // buf for recv data
    char statusBuf[10]; // buf status login
    std::string userInput;
    std::vector <timeoutSend> timeoutList;

    std::vector<fileNode>fileLst(MAX_FILE_TXT);

    std:: queue <msg_text> qSend;
    bool reconnect = true;
    //init file list
    for(int i = 0; i < MAX_FILE_TXT; i++){
        fileLst[i].id = -1;
        fileLst[i].isSend = false;
        fileLst[i].timeModify = 0;
    }

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
            //std::cout << "numsend "<< num << "\n";
            loginId = msgSend.ID;

            //socket non-blocking mode
            fcntl(socket, F_SETFL, O_NONBLOCK);
            // main thread for send msg
            // press # for logout
            std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend),ref(fileLst));
            std::thread sendMsgThread(sendMsg,socket,ref(qSend),ref(timeoutList));
            std::thread recvThread(recvMsg,bufrcv,socket,ref(timeoutList));
            std::thread timeoutThr(timeoutThread,socket,ref(timeoutList),ref(qSend));



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
