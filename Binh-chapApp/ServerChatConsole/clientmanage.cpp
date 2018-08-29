#include "clientmanage.h"

ClientManage::ClientManage()
{

}
//login client
void ClientManage::mapClientWithSocket(clientNode clientList[],int socketfd,char *buf,fd_set listener){
    bool flagCheck = false;
    for(int i = 0; i < MAX_CLIENT; i++){
        // gui cac msg khi client offline

        //(neu full client gui msg thong bao va close connect)

        // client da ton tai -> status on -> kiem tra queue msg co ng gui khong
        if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == false){
            flagCheck = true;
            clientList[i].status = true;
            clientList[i].socketfd = socketfd;
            break;
            //configClientFlag = true;

        }
        // da ton tai username do roi
        else if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == true){
            std::cout<<"client is existed\n";
            flagCheck = true;
            close(socketfd);
            break;
            FD_CLR(socketfd,&listener);
        }
        // client chua ton tai -> tao client moi
        else if(clientList[i].id == -1){
            clientList[i].name = new char[strlen(buf)+1];
            strcpy(clientList[i].name,buf);
            std::cout<<"client connect info:\n";
            std::cout<<"client name " << std::string(clientList[i].name,0,strlen(clientList[i].name))<<"\n";
            std::cout<<"socket "<<socketfd<<"\n";
            clientList[i].id = i;
            clientList[i].socketfd = socketfd;
            //clientSocket = -1;
            clientList[i].status = true;
            flagCheck = true;
            //configClientFlag = true;
            //send(clientList[i].socketfd ,"ACK",4,0);
            break;
        }
    }

    if(flagCheck == false){
        std::cout<<"server is full client\n";
    }
}

//send Msg To client

void ClientManage::sendMsgToClient(clientNode clientList[],char *msg, int socketfd){
    //char nameClientSend[20];
    //char nameClientRecv[20];
    //char msgData[4096];
    char bufSend[4096];
    memset(bufSend,0,4096);
    char *nameClientSend = std::strtok(msg,"/");
    std::string msgData = (std::string(msg,0,strlen(msg))).substr(strlen(nameClientSend),strlen(msg));
    bool flag = false;
    for(int i = 0; i < MAX_CLIENT ; i++){
        if(clientList[i].socketfd == socketfd){
            strcat(bufSend,clientList[i].name);
            strcat(bufSend,": ");
            strcat(bufSend,msgData.c_str());
            break;
        }
    }

    if(strcmp(nameClientSend,"all")==0){
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].status == true && clientList[i].socketfd != socketfd){

                send(clientList[i].socketfd ,bufSend,strlen(bufSend),0);
            }
        }
    }
    else{
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(strcmp(clientList[i].name,nameClientSend) == 0){
                if(clientList[i].status == true){
                    send(clientList[i].socketfd ,bufSend,strlen(bufSend),0);
                    flag = true;
                }
                else{
                    //store msg
                    std::ofstream outfile;
                    outfile.open(strcat(clientList[i].name,".txt"),std::ios::out | std::ios::app);
                    time_t     now;
                    struct tm  ts;
                    char       buf[80];
                    // Get current time
                    time(&now);
                    // Format time, "ddd yyyy-mm-dd hh:mm:ss zzz"
                    ts = *localtime(&now);
                    strftime(buf, sizeof(buf), "%a %Y-%m-%d %H:%M:%S", &ts);
                    outfile << bufSend <<" "<< std::endl;
                    outfile.close();
                    flag = true;
                }
                break;
            }
        }
        if(flag == false){
            std::cout<< "client does not exist or wrong format\n";
        }
    }

}



