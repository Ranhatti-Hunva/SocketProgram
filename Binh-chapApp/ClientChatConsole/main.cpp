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
            std::cout<<"bytes recv "<<bytesRecv<<"\n";

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
//------------Read All Bytes in file ----------------------------------------------------
std::string ReadAllBytes(const char * filename, int * read)
{
    std::ifstream ifs(filename, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();

    // What happens if the OS supports really big files.
    // It may be larger than 32 bits?
    // This will silently truncate the value/
    int length = pos;

    // Manuall memory management.
    // Not a good idea use a container/.
    char *pChars = new char[pos];
    ifs.seekg(0, std::ios::beg);
    ifs.read(pChars, length);

    // No need to manually close.
    // When the stream goes out of scope it will close the file
    // automatically. Unless you are checking the close for errors
    // let the destructor do it.
    ifs.close();
    *read = length;

    return std::string(pChars,0,length);
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

//------------add file to Q send---------------------------------------------------------
/*void addFileList(std::vector<fileNode>&fileList,std::string fileName){
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
*/
//----send msg in file ------------------------------------------------------------------
void sendMsgInFile(std::string name,
                   std::string filePath,
                   std::queue<msg_text>&msgQ){

    std::ifstream ifs(filePath, std::ios::binary|std::ios::ate);
    std::ifstream::pos_type pos = ifs.tellg();
    ifs.seekg(0, std::ios::beg);
    int block = 1024; // 1 KB

    int numberBlock = (pos/block) + 1;
    //std::cout<<"pos number  "<<pos<<"\n";
    //std::cout<<"block number  "<<numberBlock<<"\n";
    for(int i = 0; i < numberBlock; i++){
        char *pChars = new char[block];
        memset(pChars,0,block);

        ifs.read(pChars,block);
        //std::cout<<"block   "<< i << "-----------------\n"<<std::string(pChars,0,1024)<<"\n";
        msg_text msgSend;
        msgSend.type_msg = MSG;
        msgSend.msg.assign(name + "/" +pChars);
        mtx.lock();
        msgQ.push(msgSend);
        usleep(1000);
        mtx.unlock();
        delete []pChars;
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
                    bool &stopFile){

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
        sendMsgInFile(name,fullName,ref(msgQ));
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
                    sendMsgInFile(name,fullName,ref(msgQ));
                }
            }


        }
    }

}


//----thread nhan tu console-------------------------------------------------------------

void cinFromConsole(int socket,
                    std::queue<msg_text>&msgQ,
                    thread_pool &pool){
    //int socket;std::queue<msg_text>msgQ;

    std::string folderPath = "../readFile";
    msg_text msgSend;
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    bool stopFile = false;

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
                stopFile = false;
                std::string username = userInput.substr(5,userInput.size());

                if(!username.empty()){
                    //add thread
                    std::cout<<"usr name "<<username<<"\n";
                    pool.enqueue([&,username]{
                        sendFileThread(username,ref(msgQ),stopFile);
                    });
                }

                /*               std::string dir;
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

                            msgQ.push(msgSend);
                            mtx.unlock();
                            delete []pChars;
                        }
                        ifs.close();

                        fileList[i].isSend = true;
                    }
                }
*/


            }
            else if(strcmp(userInput.c_str(),"un_file/") == 0){
                stopFile = true;
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
            int buferSize = msgQ.front().msg.length()+10;

            unsigned char *buf = new unsigned char [buferSize];
            memset(buf,0,buferSize);
            //std::unique_ptr <unsigned char> buf (new unsigned char [buferSize]);
            handleMsg.packed_msg(msgQ.front(),buf);

            //            std::cout<<"\nlbi  --- "<<buferSize<<" "<< msgQ.size()<<"\n";
            //            for(int i = 0; i < buferSize;i++){

            //                printf("%c",msgQ.front().msg.c_str()[i]);
            //            }
            std::cout<<"\n";
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
    thread_pool pool(10);


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
            //std::thread cinConsoleThread(cinFromConsole,socket,ref(qSend),ref(pool));

            pool.enqueue([&]{
                cinFromConsole(socket,ref(qSend),pool);
            });
            std::thread sendMsgThread(sendMsg,socket,ref(qSend),ref(timeoutList));
            std::thread recvThread(recvMsg,bufrcv,socket,ref(timeoutList));
            std::thread timeoutThr(timeoutThread,socket,ref(timeoutList),ref(qSend));



            //stop all thread before reconnect
            sendMsgThread.join();
            recvThread.join();
            //cinConsoleThread.join();
            timeoutThr.join();
        }

        reconnect = isReconnect(ref(timeoutList));

    }
    return 0;
}
