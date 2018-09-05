#include "clientmanage.h"
#include "handlemsg.h"
#include <mutex>
#include <map>

std::mutex mtx;

ClientManage::ClientManage()
{

}
//login client
int ClientManage::mapClientWithSocket(std::vector <clientNode> &clientList,
                                      int socketfd, char * buf, fd_set &fds){
    bool flagCheck = false;
    HandleMsg handle;
    int socketExist = -1;

//    std::map<std::string,int> mapClient;
//    std::map<std::string,int>::iterator it;

//    for(int i = 0; i<MAX_CLIENT; i++){
//        if(clientList[i].id != -1){
//            mapClient.insert(std::pair<std::string,int>(clientList[i].name,clientList[i].id));
//        }
//    }

    for(int i = 0; i < MAX_CLIENT; i++){
        // gui cac msg khi client offline

        //(neu full client gui msg thong bao va close connect)

        // client da ton tai -> status on -> kiem tra queue msg co ng gui khong
        if((clientList[i].id != -1) && (strcmp(clientList[i].name,buf) == 0)
                && (clientList[i].status == false)){
            flagCheck = true;
            //send(socketfd,"success",7,0);

            //check socket co mo hay ko truong hop timeout
            if(FD_ISSET(clientList[i].socketfd,&fds) == 0){
                int error_code;
                socklen_t optlen = sizeof(error_code);

                int res = getsockopt(clientList[i].socketfd, SOL_SOCKET, SO_ERROR, &error_code, &optlen);
                //socket is open
                if(error_code==0 && res == 0){
                    socketExist = clientList[i].socketfd;
                }
            }


            clientList[i].status = true;
            clientList[i].socketfd = socketfd;
            usleep(1000);
            std::ifstream infile;
            char * filename = new char[strlen(clientList[i].name)];
            strcpy(filename,clientList[i].name);
            //std::string e;
            infile.open(strcat(filename,".txt"));

            if(infile.is_open() == true){
                while(!infile.eof()){
                    //char * a = new char [1024];
                    std::string line;
                    getline (infile,line);
                    //infile.getline(a,1024);

                    //dong goi data
                    if(!line.empty()){
                        struct msg_text msgSend;
                        msgSend.type_msg = MSG;

                        //msgSend.msg.assign(std::string(a,0,strlen(a)));
                        msgSend.msg.assign(line);
                        unsigned char buffer[msgSend.msg.length()+9];
                        HandleMsg handleMsg;
                        handleMsg.packed_msg(msgSend,buffer);

                        //qRecv.push(msgSend);
                        usleep(1000);
                        send(clientList[i].socketfd,buffer,sizeof(buffer),0);


                        std::cout<<line<<"\n";
                    }


                    //std::cout<<std::string(a,0,1024)<<"\n";
                    //delete []a;
                }
                infile.close();
                std::ofstream ofs;
                ofs.open(filename, std::ofstream::out | std::ofstream::trunc);
                ofs.close();
            }


            break;
            //configClientFlag = true;

        }
        // da ton tai username do roi
        else if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == true){
            //std::cout<<"client is existed\n";
            flagCheck = true;
            //send(socketfd,"existed",7,0);

            //mtx.lock();
            //close(clientList[i].socketfd);
            //FD_CLR(clientList[i].socketfd,&listener);
            //mtx.unlock();
            socketExist = clientList[i].socketfd;
            clientList[i].socketfd = socketfd;

            break;

        }
        // client chua ton tai -> tao client moi
        else if(clientList[i].id == -1){
            clientList[i].name = new char[strlen(buf)+1];
            strcpy(clientList[i].name,buf);
            std::cout<<"Client connect info: ";
            std::cout<<"Name " << std::string(clientList[i].name,0,strlen(clientList[i].name))<<" - ";
            std::cout<<"socket "<<socketfd<<"\n";
            clientList[i].id = i;
            clientList[i].socketfd = socketfd;
            //clientSocket = -1;
            clientList[i].status = true;
            flagCheck = true;
            //send(socketfd,"success",7,0);
            break;
        }
    }

    if(flagCheck == false){
        //send(socketfd,"full",5,0);
        std::cout<<"socket full "<<socketfd<<"\n";
        std::cout<<"server is full client\n";
    }
    return socketExist;
}

//send Msg To client--------------------------------------------------------------------

void ClientManage::sendMsgToClient(std::vector <clientNode> &clientList,
                                   char *msg, int socketfd,
                                   std::vector <timeoutNode> &timeoutList){

    if(msg != nullptr){
        struct timeval tv;
        //std::unique_ptr<char>a(new char[10]);
        std::unique_ptr<char> bufSend (new char[2048]);
        //char * bufSend = new char [2048];
        memset(bufSend.get(),0,2048);

        std::unique_ptr<char> dataMsg (new char(strlen(msg)));
        //char *dataMsg = new char [strlen(msg)];
        strcpy(dataMsg.get(),msg);
        char *nameClientSend = std::strtok(msg,"/");

        std::cout<<"\nnameClientSend : "<< std::string(nameClientSend,0,strlen(nameClientSend)) << "\n";


        bool flag = false;
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].socketfd == socketfd && clientList[i].status == true){

                if(strlen(nameClientSend)+1 < strlen(dataMsg.get())){
                    //get name of client send msg
                    std::string msgData = std::string(dataMsg.get(),0,strlen(dataMsg.get()))
                            .substr(strlen(nameClientSend)+1,strlen(dataMsg.get()));

                    std::cout<<"\ndata msg: "<< msgData << "\n";

                    strcat(bufSend.get(),clientList[i].name);
                    strcat(bufSend.get(),": ");
                    strcat(bufSend.get(),msgData.c_str());
                }
                else{
                    std::cout<<"\nwrong format\n";
                }

                break;
            }
        }
        //delete [] dataMsg;


        //dong goi data
        struct msg_text msgSend;
        msgSend.type_msg = MSG;

        msgSend.msg.assign(std::string(bufSend.get(),0,strlen(bufSend.get())));

        unsigned char buffer[msgSend.msg.length()+9];
        HandleMsg handleMsg;
        handleMsg.packed_msg(msgSend,buffer);

//        std::cout<<"send buf\n";
//        for(int i = 0; i< msgSend.msg.length()+9;i++){
//            std::cout<<(unsigned int)buffer[i]<<" ";
//        }
//        std::cout<<"\n";

        std::cout<<"\nnameClientSend : "<< std::string(nameClientSend,0,strlen(nameClientSend)) << "\n";
        //std::cout<<"strcmp all: "<<strcmp(nameClientSend,"all")<<"\n";


        if(strcmp(nameClientSend,"all")==0){
            for(int i = 0; i < MAX_CLIENT ; i++){
                if(clientList[i].status == true && clientList[i].socketfd != socketfd){
                    flag = true;
                    // dong goi truoc khi send
                    send(clientList[i].socketfd ,buffer,sizeof(buffer),0);
                    //add Q timeout rsp
                    gettimeofday(&tv, nullptr);
                    timeoutNode to;
                    to.timeout = tv.tv_sec*1000 + tv.tv_usec/1000 ;
                    to.msgID = msgSend.ID;
                    to.socket = clientList[i].socketfd;
                    timeoutList.push_back(to);

                }
            }
        }
        else{
            for(int i = 0; i < MAX_CLIENT ; i++){
                if(clientList[i].id != -1 && clientList[i].status == false){
                    std::cout<<std::string(clientList[i].name,0,20)<<" off\n";
                    //std::cout<<"str cmp : "<<strcmp(clientList[i].name,nameClientSend)<<"\n";
                }
                if(strcmp(clientList[i].name,nameClientSend) == 0){
                    if(clientList[i].status == true){
                        //dam bao rsp se gui truoc msg trong truong hop client gui msg cho chinh no
                        usleep(10000);
                        send(clientList[i].socketfd ,buffer,sizeof(buffer),0);
                        flag = true;
                        gettimeofday(&tv, nullptr);
                        timeoutNode to;
                        to.timeout = tv.tv_sec*1000 + tv.tv_usec/1000 ;
                        to.msgID = msgSend.ID;
                        to.socket = clientList[i].socketfd;
                        timeoutList.push_back(to);
                    }
                    else{
                        //store msg
                        std::ofstream outfile;
                        char * filename = new char[strlen(clientList[i].name)];
                        strcpy(filename,clientList[i].name);
                        outfile.open(strcat(filename,".txt"),std::ios::out | std::ios::app);
                        time_t     now;
                        struct tm  ts;
                        char       buf[80];
                        // Get current time
                        time(&now);
                        // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                        ts = *localtime(&now);
                        strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
                        outfile << bufSend.get() <<" --- "<< buf <<std::endl;
                        outfile.close();
                        std::cout<< "xxxxxxxxxxxxx\n";
                        flag = true;
                    }
                    break;
                }
            }

        }

        if(flag == false){
            std::cout<< "client does not exist or wrong format\n";
        }
        //delete [] bufSend;
    }

}



