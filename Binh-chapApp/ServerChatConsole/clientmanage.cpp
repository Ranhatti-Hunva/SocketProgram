#include "clientmanage.h"
#include "handlemsg.h"
#include <mutex>

std::mutex mtx;

ClientManage::ClientManage()
{

}
//login client
int ClientManage::mapClientWithSocket(std::vector <clientNode> &clientList,
                                      int socketfd, char * buf, std::queue<msg_text> &qRecv){
    bool flagCheck = false;
    HandleMsg handle;
    int socketExist = -1;
    for(int i = 0; i < MAX_CLIENT; i++){
        // gui cac msg khi client offline

        //(neu full client gui msg thong bao va close connect)

        // client da ton tai -> status on -> kiem tra queue msg co ng gui khong
        if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == false){
            flagCheck = true;
            //send(socketfd,"success",7,0);

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

//send Msg To client

void ClientManage::sendMsgToClient(std::vector <clientNode> &clientList,char *msg, int socketfd){
    //char nameClientSend[20];
    //char nameClientRecv[20];
    //char msgData[4096];
    char * bufSend = new char [2048];
    memset(bufSend,0,2048);
    char *dataMsg = new char [strlen(msg)];
    strcpy(dataMsg,msg);
    char *nameClientSend = std::strtok(msg,"/");
    //std::string msgData = (std::string(msg,0,strlen(msg))).substr(strlen(nameClientSend),strlen(msg));
    //    if(nameClientSend == nullptr){
    //        std::cout << " oh yeeh\n";
    //    }

    std::cout<<"\nnameClientSend : "<< std::string(nameClientSend,0,strlen(nameClientSend)) << "\n";
    //std::cout<<"\ndata msg: "<< msgData << "\n";


    bool flag = false;
    for(int i = 0; i < MAX_CLIENT ; i++){
        if(clientList[i].socketfd == socketfd && clientList[i].status == true){

            if(strlen(nameClientSend)+1 < strlen(dataMsg)){
                std::string msgData = std::string(dataMsg,0,strlen(dataMsg)).substr(strlen(nameClientSend)+1,strlen(dataMsg));
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
    delete [] dataMsg;


    //dong goi data
    struct msg_text msgSend;
    msgSend.type_msg = MSG;

    msgSend.msg.assign(std::string(bufSend,0,strlen(bufSend)));
    unsigned char buffer[msgSend.msg.length()+9];
    HandleMsg handleMsg;
    handleMsg.packed_msg(msgSend,buffer);

    std::cout<<"\nnameClientSend : "<< std::string(nameClientSend,0,strlen(nameClientSend)) << "\n";
    std::cout<<"strcmp all: "<<strcmp(nameClientSend,"all")<<"\n";


    if(strcmp(nameClientSend,"all")==0){
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].status == true && clientList[i].socketfd != socketfd){

                // dong goi truoc khi send
                send(clientList[i].socketfd ,buffer,sizeof(buffer),0);
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
                    usleep(1000);
                    send(clientList[i].socketfd ,buffer,sizeof(buffer),0);
                    flag = true;
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
                    outfile << bufSend <<" --- "<< buf <<std::endl;
                    outfile.close();
                    std::cout<< "xxxxxxxxxxxxx\n";
                    flag = true;
                }
                break;
            }
        }
        if(flag == false){
            std::cout<< "client does not exist or wrong format\n";
        }
    }
    delete [] bufSend;

}



