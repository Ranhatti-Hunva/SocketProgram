#include "clientmanage.h"
#include "handlemsg.h"
#include <mutex>
#include <map>



ClientManage::ClientManage()
{

}
//------login client---------------------------------------------------------------------
int ClientManage::mapClientWithSocket(std::vector <clientNode> &clientList,
                                      int socketfd, char * buf, fd_set &fds){
    bool flagCheck = false;
    HandleMsg handle;
    int socketExist = -1;

    std::map<std::string,int> mapClient;
    std::map<std::string,int>::iterator it;

    for(int i = 0; i< MAX_CLIENT; i++){
        if(clientList[i].id != -1){
            mapClient.insert(std::pair<std::string,int>(clientList[i].name,clientList[i].id));
        }
    }

    it = mapClient.find(buf);
    if (it != mapClient.end()){
        //mapClient.
        //        it->first;
        //        std::cout<<"has name in list \n";
        //client da ton tai
        int posClient = it->second;
        if(clientList[posClient].status == false){
            flagCheck = true;
            //send(socketfd,"success",7,0);

            //check socket co mo hay ko truong hop timeout
            if(FD_ISSET(clientList[posClient].socketfd,&fds) == 0){
                int error_code;
                socklen_t optlen = sizeof(error_code);

                int res = getsockopt(clientList[posClient].socketfd, SOL_SOCKET, SO_ERROR, &error_code, &optlen);
                //socket is open
                if(error_code==0 && res == 0){
                    socketExist = clientList[posClient].socketfd;
                }
            }


            clientList[posClient].status = true;
            clientList[posClient].socketfd = socketfd;
            usleep(1000);
            std::ifstream infile;
            char * filename = new char[strlen(clientList[posClient].name)];
            strcpy(filename,clientList[posClient].name);
            //std::string e;
            infile.open(strcat(filename,".txt"));

            if(infile.is_open() == true){
                while(!infile.eof()){
                    std::string line;
                    getline (infile,line);

                    //dong goi data
                    if(!line.empty()){
                        struct msg_text msgSend;
                        msgSend.type_msg = MSG;

                        msgSend.msg.assign(line);
                        unsigned char buffer[msgSend.msg.length()+9];
                        HandleMsg handleMsg;
                        handleMsg.packed_msg(msgSend,buffer);

                        //usleep(1000);
                        send(clientList[posClient].socketfd,buffer,sizeof(buffer),0);

                        std::cout<<line<<"\n";
                    }

                    //delete []a;
                }
                infile.close();
                std::ofstream ofs;
                ofs.open(filename, std::ofstream::out | std::ofstream::trunc);
                ofs.close();
            }
        }
        else{
            flagCheck = true;
            socketExist = clientList[posClient].socketfd;
            clientList[posClient].socketfd = socketfd;
        }

    }
    else{
        //add new client
        int posClient = mapClient.size();
        clientList[posClient].id =  posClient;
        clientList[posClient].name = new char[strlen(buf)+1];
        strcpy(clientList[posClient].name,buf);
        clientList[posClient].socketfd = socketfd;
        clientList[posClient].status = true;
        std::cout<<"Client connect info: ";
        std::cout<<"Name " << std::string(clientList[posClient].name,
                                          0,strlen(clientList[posClient].name))<<" - ";
        std::cout<<"socket "<<socketfd<<"\n";
        flagCheck = true;
    }
    if(flagCheck == false){
        //send(socketfd,"full",5,0);
        std::cout<<"socket full "<<socketfd<<"\n";
        std::cout<<"server is full client\n";
    }
    return socketExist;

}

//------------send Msg To client---------------------------------------------------------

void ClientManage::sendMsgToClient(std::vector <clientNode> &clientList,
                                   char *msg, int socketfd,
                                   std::vector <timeoutNode> &timeoutList){


    struct timeval tv;
    //std::unique_ptr<char>a(new char[10]);
    //std::unique_ptr<char> bufSend (new char[2048]);
    char * bufSend = new char [2048];
    memset(bufSend,0,2048);

    //std::unique_ptr<char> dataMsg (new char(strlen(msg)));

    //std::cout<<"str len "<<strlen(msg)<<"\n";
    char *dataMsg = new char [strlen(msg)];
    strcpy(dataMsg,msg);
    char *nameClientSend = std::strtok(msg,"/");

    std::cout<<"\nnameClientSend : "<< std::string(nameClientSend,0,strlen(nameClientSend)) << "\n";


    bool flag = false;


    std::map<std::string,int> mapClient;
    std::map<std::string,int>::iterator it;

    for(int i = 0; i< MAX_CLIENT; i++){
        if(clientList[i].id != -1){
            mapClient.insert(std::pair<std::string,int>(clientList[i].name,clientList[i].id));
        }
    }

    //assign ten ng gui
    for(int i = 0; i < MAX_CLIENT ; i++){
        if(clientList[i].socketfd == socketfd && clientList[i].status == true){

            if(strlen(nameClientSend)+1 < strlen(dataMsg)){
                //get name of client send msg
                std::string msgData = std::string(dataMsg,0,strlen(dataMsg))
                        .substr(strlen(nameClientSend)+1,strlen(dataMsg));

                std::cout<<"\ndata msg: "<< msgData << "\n";

                strcat(bufSend,clientList[i].name);
                strcat(bufSend,": ");
                strcat(bufSend,msgData.c_str());
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

    msgSend.msg.assign(std::string(bufSend,0,strlen(bufSend)));

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
                //mtx.lock();
                gettimeofday(&tv, nullptr);
                timeoutNode to;
                to.timeout = tv.tv_sec*1000 + tv.tv_usec/1000 ;
                to.msgID = msgSend.ID;
                to.socket = clientList[i].socketfd;
                timeoutList.push_back(to);
                //mtx.unlock();

            }
        }
    }
    else{
        it = mapClient.find(nameClientSend);
        if (it != mapClient.end()){
            int posClient = it->second;
            //std::cout<<"pos "<<posClient<<"\n";
            if(clientList[posClient].status == true){
                //dam bao rsp se gui truoc msg trong truong hop client gui msg cho chinh no
                //usleep(1000);
                send(clientList[posClient].socketfd ,buffer,sizeof(buffer),0);
                flag = true;
                mtx.lock();
                gettimeofday(&tv, nullptr);
                timeoutNode to;
                to.timeout = tv.tv_sec*1000 + tv.tv_usec/1000 ;
                to.msgID = msgSend.ID;
                to.socket = clientList[posClient].socketfd;
                timeoutList.push_back(to);
                mtx.unlock();
            }
            else{
                std::ofstream outfile;
                char * filename = new char[strlen(clientList[posClient].name)];
                strcpy(filename,clientList[posClient].name);
                outfile.open(strcat(filename,".txt"),std::ios::out | std::ios::app);
                time_t     now;
                struct tm  ts;
                char       buf[80];
                // Get current time
                time(&now);
                // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                ts = *localtime(&now);
                strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
                outfile << bufSend <<" --- "<< buf <<std::endl;
                outfile.close();
                std::cout<< "store msg !\n";
                flag = true;
            }
        }
        else{
            std::cout<< "client does not exist or wrong format\n";
        }

    }
    delete [] dataMsg;
    delete [] bufSend;

}



