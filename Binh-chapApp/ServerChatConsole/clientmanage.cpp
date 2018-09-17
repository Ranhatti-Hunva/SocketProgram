#include "clientmanage.h"

//---------------------------------------------------------------------------------------
ClientManage::ClientManage()
{

}

//------login client---------------------------------------------------------------------
int ClientManage::mapClientWithSocket(std::vector <clientNode> &clientList,
                                      int socketfd, char * buf, fd_set &fds,
                                      std::mutex &mt){
    bool flagCheck = false;
    HandleMsg handle;
    int socketExist = -1;
    mt.lock();
    std::map<std::string,int> mapClient;
    std::map<std::string,int>::iterator it;

    for(int i = 0; i< MAX_CLIENT; i++){
        if(clientList[i].id != -1){
            mapClient.insert(std::pair<std::string,int>(clientList[i].name,clientList[i].id));
        }
    }

    it = mapClient.find(buf);
    if (it != mapClient.end()){
        //client da ton tai
        int posClient = it->second;
        if(clientList[posClient].status == false){
            flagCheck = true;

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
            //readfile client offline
            clientList[posClient].readfile = true;
        }
        else{
            //client online in another console -> close one
            flagCheck = true;
            clientList[posClient].readfile = false;
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

        clientList[posClient].readfile = false;
        flagCheck = true;
    }

    if(flagCheck == false){
        //send(socketfd,"full",5,0);
        std::cout<<"socket full "<<socketfd<<"\n";
        std::cout<<"server is full client\n";
    }
    mt.unlock();
    return socketExist;
}

//------ send msg in file for client offline---------------------------------------------
void ClientManage::sendOffClient(msgQueue &qSend,
                                 std::mutex &mt,
                                 std::vector <clientNode> &clientList){
    for(int i = 0; i< clientList.size() ; i++){
        mt.lock();

        if(clientList[i].readfile == true){
            //std::cout<<"name "<<std::string(clientList[i].name,0,20)<<" co vao day\n" ;
            std::ifstream infile;
            char * filename = new char[strlen(clientList[i].name)];
            strcpy(filename,clientList[i].name);

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
                        uint8_t buffer[msgSend.msg.length()+9];
                        HandleMsg handleMsg;
                        handleMsg.packed_msg(msgSend,buffer);


                        sendNode node;
                        node.len = sizeof(buffer);

                        node.buf = new uint8_t [node.len];
                        memcpy(node.buf,buffer,node.len);

                        node.socket = clientList[i].socketfd;
                        node.msgID = msgSend.ID;
                        qSend.pushQ(node);
                    }

                }
                infile.close();
                std::ofstream ofs;
                ofs.open(filename, std::ofstream::out | std::ofstream::trunc);
                ofs.close();

            }
            clientList[i].readfile = false;
        }
        mt.unlock();
    }
}


//------------send Msg To client---------------------------------------------------------

void ClientManage::sendMsgToClient(std::vector <clientNode> &clientList,
                                   char *msg, int socketfd,
                                   std::vector <timeoutNode> &timeoutList,
                                   msgQueue &qSend,std::mutex &mt){
    //std::unique_ptr<char> bufSend (new char[2048]);
    char * bufSend = new char [4096];
    memset(bufSend,0,4096);

    //std::unique_ptr<char> dataMsg (new char(strlen(msg)));
    char *dataMsg = new char [strlen(msg)];
    strcpy(dataMsg,msg);
    std::string strMsg;
    strMsg.assign(std::string(msg,0,strlen(msg)));

    std::size_t found = strMsg.find_first_of("/");

    if(found != std::string::npos){
        char nameClient[found];
        strcpy(nameClient,strMsg.substr(0,found).c_str());
        std::cout<<"\n==> Ten nguoi muon gui "<<std::string(nameClient,0,found)<<"\n";
        std::map<std::string,int> mapClient;
        std::map<std::string,int>::iterator it;
        for(int i = 0; i< MAX_CLIENT; i++){
            if(clientList[i].id != -1){
                mapClient.insert(std::pair<std::string,int>(clientList[i].name,clientList[i].id));
            }
        }

        // tim ten nguoi gui
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].socketfd == socketfd && clientList[i].status == true){
                if(found+1 < strlen(dataMsg)){
                    //get name of client send msg
                    std::string msgData = std::string(dataMsg,0,strlen(dataMsg))
                            .substr(found+1,strlen(dataMsg));
                    std::cout<<"data msg: "<< msgData << "\n";
                    strcat(bufSend,clientList[i].name);
                    strcat(bufSend,">");
                    strcat(bufSend,msgData.c_str());
                }
                else{
                    std::cout<<"\nwrong format\n";
                }
                break;
            }
        }

        //dong goi data
        struct msg_text msgSend;
        msgSend.type_msg = MSG;
        msgSend.msg.assign(std::string(bufSend,0,strlen(bufSend)));
        uint bufferSize = msgSend.msg.length() + 9;
        uint8_t buffer[bufferSize];
        HandleMsg handleMsg;
        handleMsg.packed_msg(msgSend,buffer);
        if(strcmp(nameClient,"all") == 0){
            for(int i = 0; i < MAX_CLIENT ; i++){
                if(clientList[i].status == true && clientList[i].socketfd != socketfd){

                    addQsend(clientList[i].socketfd ,
                             timeoutList,qSend,mt,
                             buffer,sizeof (buffer),msgSend.ID);

                }
            }
        }
        else{
            it = mapClient.find(nameClient);

            if (it != mapClient.end()){
                int posClient = it->second;
                //std::cout<<"pos "<<posClient<<"\n";

                if(clientList[posClient].status == true){

                    addQsend(clientList[posClient].socketfd,
                             timeoutList,qSend,mt,
                             buffer,sizeof (buffer),msgSend.ID);
                }
                else{
                    storeMsg(clientList[posClient].name,bufSend);
                }
            }
            else{
                std::cout<< "client does not exist\n";
            }

        }

        delete [] dataMsg;
        delete [] bufSend;
    }
    else{
        std::cout<<"wrong format\n";
    }
}

//------------add MSG to Queue ----------------------------------------------------------
void ClientManage::addQsend (int socketfd,
                             std::vector <timeoutNode> &timeoutList,
                             msgQueue &qSend,
                             std::mutex &mt,
                             uint8_t *buffer,
                             uint lenMsg,
                             uint ID)
{
    struct timeval tv;

    mt.lock();

    sendNode node;
    node.buf = new uint8_t [lenMsg];
    memcpy(node.buf,buffer,lenMsg);
    node.len = lenMsg;
    node.socket = socketfd;
    node.msgID = ID;

    qSend.pushQ(node);

    gettimeofday(&tv, nullptr);

    timeoutNode to;
    to.timeout = tv.tv_sec*1000 + tv.tv_usec/1000 ;
    to.msgID = ID;
    to.socket = socketfd;

    timeoutList.push_back(to);

    mt.unlock();

}

//------------store MSG in file ---------------------------------------------------------
void ClientManage::storeMsg (char *name, char *bufSend){
    std::ofstream outfile;
    char * filename = new char[strlen(name)];
    strcpy(filename,name);
    outfile.open(strcat(filename,".txt"),std::ios::out | std::ios::app);
    time_t     now;
    struct tm  ts;
    char       buf[80];
    // Get current time
    time(&now);
    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
    ts = *localtime(&now);
    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
    outfile << bufSend /*<<" --- "<< buf */<<std::endl;
    outfile.close();
    std::cout<< "store msg !\n";
}
