#include "clientmanage.h"
#include "handlemsg.h"
ClientManage::ClientManage()
{

}
//login client
void ClientManage::mapClientWithSocket(std::vector <clientNode> &clientList,int socketfd,char * buf){
    bool flagCheck = false;
    HandleMsg handle;
    for(int i = 0; i < MAX_CLIENT; i++){
        // gui cac msg khi client offline

        //(neu full client gui msg thong bao va close connect)

        // client da ton tai -> status on -> kiem tra queue msg co ng gui khong
        if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == false){
            flagCheck = true;
            send(socketfd,"success",7,0);

            clientList[i].status = true;
            clientList[i].socketfd = socketfd;
            std::ifstream infile;
            char * filename = new char[strlen(clientList[i].name)];
            strcpy(filename,clientList[i].name);
            std::string e;
            infile.open(strcat(filename,".txt"));
            while(!infile.eof()){
                char a[1024];
                infile.getline(a,1024);
                send(clientList[i].socketfd,a,strlen(a),0);
                std::cout<<std::string(a,0,250)<<"\n";
            }
            infile.close();
            std::ofstream ofs;
            ofs.open(filename, std::ofstream::out | std::ofstream::trunc);
            ofs.close();
            //
            //
            //               infile.open("vietjack.dat");

            //               cout << "\n===========================\n" ;
            //               cout << "Doc du lieu co trong file!" << endl;
            //               infile >> data;

            break;
            //configClientFlag = true;

        }
        // da ton tai username do roi
        else if(clientList[i].id != -1 && strcmp(clientList[i].name,buf) == 0
                && clientList[i].status == true){
            std::cout<<"client is existed\n";
            flagCheck = true;
            send(socketfd,"existed",7,0);
            break;

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
            send(socketfd,"success",7,0);
            break;
        }
    }

    if(flagCheck == false){
        send(socketfd,"full",5,0);
        std::cout<<"socket full "<<socketfd<<"\n";
        std::cout<<"server is full client\n";
    }
}

//send Msg To client

void ClientManage::sendMsgToClient(std::vector <clientNode> &clientList,char *msg, int socketfd){
    //char nameClientSend[20];
    //char nameClientRecv[20];
    //char msgData[4096];
    char bufSend[4096];
    memset(bufSend,0,4096);
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
        if(clientList[i].socketfd == socketfd){

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

    if(strcmp(nameClientSend,"all")==0){
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].status == true && clientList[i].socketfd != socketfd){

                // dong goi truoc khi send
                send(clientList[i].socketfd ,bufSend,strlen(bufSend),0);
            }
        }
    }
    else{
        for(int i = 0; i < MAX_CLIENT ; i++){
            if(clientList[i].id != -1 && clientList[i].status == false){
                std::cout<<std::string(clientList[i].name,0,20)<<" off\n";
                std::cout<<"str cmp : "<<strcmp(clientList[i].name,nameClientSend)<<"\n";
            }
            if(strcmp(clientList[i].name,nameClientSend) == 0){
                if(clientList[i].status == true){
                    send(clientList[i].socketfd ,bufSend,strlen(bufSend),0);
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

}



