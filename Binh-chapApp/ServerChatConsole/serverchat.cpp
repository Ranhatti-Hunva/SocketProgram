#include "serverchat.h"

//-----------------------------------------------------------------------------
ServerChat::ServerChat(char* ipAddress, char* port)
    :m_ipAddr(ipAddress), m_port(port)
{

}
//-----------------------------------------------------------------------------
ServerChat::~ServerChat(){
    cleanUp();
}

//---Client thread proccess----------------------------------------------------
void handleClient(clientNode &cli,std::vector <clientNode> &cliArr){
    char *msg = new char[4096];
    while(1){

        int recvNum = recv(cli.socketfd,msg,4096,0);
        if(recvNum >0){
            if(cli.name == ""){
                //slipt
                for(int i = 0; i <= MAX_CLIENT; i++){
                    if(cliArr[i].socketfd == cli.socketfd){
                        //char a[400] ;
                        //strcpy(a,msg);
                        cliArr[i].name = new char[20];
                        strcpy(cliArr[i].name,msg);
                        std::cout << std::string(cli.name,0,strlen(cli.name))<<"\n";
                    }
                }

                //                cliArr
                //                strcpy(cli.name,msg);
                //                std::cout << std::string(cli.name,0,strlen(cli.name))<<"\n";
            }
            else{
                //std::cout << std::string(cli.name,0,strlen(cli.name))<<" chuoi\n";
                char bufsend[4096];
                memset(bufsend,0,4096);


                //                char input[100] = "A bird came down the walk";
                //                char *token = std::strtok(input, " ");
                //                while (token != NULL) {
                //                    std::cout << token << '\n';
                //                    token = std::strtok(NULL, " ");
                //                }

                char *toClient = std::strtok(msg,"/");

                std::cout << toClient << '\n';
                if(strcmp(toClient,"all")==0){
                    toClient = std::strtok(nullptr, "/");
                    strcat(bufsend,cli.name);
                    strcat(bufsend,": ");
                    strcat(bufsend,toClient);
                    for(int i = 0; i <= MAX_CLIENT; i++){
                        // if client valid -> send
                        if(cliArr[i].status == true){
                            //std::cout << i <<"\n";
                            //std::cout << cli.socketfd<<" ski\n" ;
                            //std::cout << cliArr[i].socketfd <<" sk\n" ;

                            if(cliArr[i].socketfd != cli.socketfd){
                                int numSend = send(cliArr[i].socketfd,bufsend,strlen(bufsend),0);
                            }
                        }
                    }
                }
                else{
                    bool flag = false;
                    for(int i = 0; i <= MAX_CLIENT; i++){

                        if(cliArr[i].status == true
                                && (strcmp(toClient,cliArr[i].name)==0)){
                            toClient = std::strtok(nullptr, "/");
                            strcat(bufsend,cli.name);
                            strcat(bufsend,": ");
                            strcat(bufsend,toClient);
                            if(cliArr[i].socketfd != cli.socketfd){
                                int numSend = send(cliArr[i].socketfd,bufsend,strlen(bufsend),0);
                                flag = true;
                            }
                        }
                    }
                    if(!flag){
                        std::cout << " client is not exist\n" ;
                    }
                }


            }
            memset(msg,0,4096);
        }
        else if(recvNum == 0){
            std::cout << "close the socket " <<cli.socketfd<<"\n";
            close(cli.socketfd);
        }

    }
}
//-----------------------------------------------------------------------------
void ServerChat::initSet(){
    memset(&hints, 0 , sizeof(hints)); // clear memory of hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(m_ipAddr,m_port, &hints, &servinfo)) != 0){
        std::cerr << "getaddrinfo: "<< gai_strerror(rv);
    }
}

//-----------------------------------------------------------------------------
int ServerChat::createSocket(){
    for( p = servinfo; p!= nullptr; p = p->ai_next){
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("server: socket");
            continue;
        }
        //check reuse addr
        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server: socket");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    if (p == nullptr){
        std:: cerr << "server: failed to bind\n";
        exit(1);
    }
    return sockfd;
}
//-----------------------------------------------------------------------------
bool ServerChat::listenSocket(int sock, int backLog){
    if(listen(sock,backLog) == -1){
        perror("listen");
        return false;
    }
    return true;
}
//-----------------------------------------------------------------------------
// wait connect
// close listening socket
// loop
// in loop:  client connect check db, has client -> show name - status on
//           hasn't client yet -> add db -> show name -status on
//           check db client has msg recv -> send msg to client
//           client disconnect -> show name - status off
//           client chat all -> broadcast msg to all client online
//           client chat personal -> send msg if online else store msg in db


//struct clientNode{
//    char *name;
//    bool status;
//    int socketfd;
//    struct msgType *msg;
//    int id;
//};
void ServerChat::mainLoop(){
    std::vector<clientNode> client(20);
    std::thread clientThread[20];
    for(int i = 0 ; i < MAX_CLIENT ; i++){
        //        client.push_back(clientNode("",false,-1,nullptr,i));
        client[i].name = "";
        client[i].status = false;
        client[i].socketfd = -1;
        //client[i].msg = nullptr;
        client[i].id = -1;
    }
    int num_client;
    int temp_id;
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    while(1){
        int newClient = -1;
        newClient = accept(sockfd,(struct sockaddr *)&remoteaddr,&addrlen);
        if(newClient == -1) continue;
        num_client = -1;
        temp_id = -1;
        int k = 0;

        for(int i = 0; i < MAX_CLIENT ; i++){
            //if( i == sockfd) continue;
            if( client[i].status == false && client[i].id == -1){
                //std::cout <<  k++ <<"\n";
                client[i].status = true;
                client[i].socketfd = newClient;
                client[i].id = i;
                temp_id = i;
                break;
            }
        }

        if(temp_id != -1){
            send(client[temp_id].socketfd, "ack", 4, 0);
            std::cout << "Client #" << client[temp_id].id << " Accepted" << std::endl;
            clientThread[temp_id] = std::thread(handleClient,std::ref(client[temp_id]),std::ref(client));
        }
    }



    //    FD_ZERO(&master);
    //    FD_ZERO(&read_fds);
    //    FD_SET(sockfd, &master);
    //    fdmax = sockfd;
    //    int flag = 0;
    //    int clientOnline = 0;
    //    std::map<int,char*> clientSock;
    //    while(1){
    //        read_fds = master;
    //        if(select(fdmax+1,&read_fds, nullptr,nullptr,nullptr) == -1){
    //            perror("select");
    //            exit(1);
    //        }
    //        for(int i = 0; i<= fdmax; i++){
    //            if(FD_ISSET(i,&read_fds)){
    //                //server listening connection
    //                if(i == sockfd){
    //                    addrlen = sizeof remoteaddr;
    //                    newfd = accept(sockfd,(struct sockaddr *)&remoteaddr,&addrlen);
    //                    if(newfd == -1){
    //                        perror("accept");
    //                    } else {
    //                        clientOnline++;
    //                        flag = 1;
    //                        FD_SET(newfd,&master);
    //                        // assign fdmax = socket of newfd
    //                        if( newfd > fdmax){
    //                            fdmax = newfd;
    //                        }
    //                        const char * network_name = inet_ntop(remoteaddr.ss_family,
    //                                                              get_in_addr((struct sockaddr*)&remoteaddr)
    //                                                              ,remoteIP,INET6_ADDRSTRLEN);
    //                        std::cout <<"new connection from" << network_name <<
    //                                    " on socket" << newfd << "\n";
    //                    }
    //                } else {
    //                    // handling data from client
    //                    if(flag == 1){
    //                        int numRecv = recv(i,buf,sizeof(buf),0);
    //                        if(numRecv > 0){
    //                            std::cout <<"client " << std::string(buf,0,numRecv) <<
    //                                        " online\n";
    //                            clientSock[i] = buf;
    //                            send(i,"ACK",4,0);
    //                        }
    //                        flag = 0;
    //                        // break;
    //                    }
    //                    else{
    //                        if((nbytes = recv(i,buf,sizeof(buf),0))<=0){
    //                            if(nbytes == 0){
    //                                // close connect
    //                                std::cout << "socket "<< i << " closed\n";
    //                            } else {
    //                                perror("revc");
    //                            }
    //                            close(i);
    //                            FD_CLR(i,&master);
    //                        } else {

    //                            //std::cout<<sizeof(buf)<<std::endl;
    //                            for(int j = 0; j <= fdmax; j++){
    //                                if (FD_ISSET(j, &master)){
    //                                    if(j!= sockfd && j != i){

    //                                        std:: cout << j <<" ooooOOOoooo"<< std::endl;
    ////                                        std::cout<< nbytes << std::endl;
    ////                                        char bufsend[4096];
    ////                                        strcpy(bufsend,clientSock[i]);
    ////                                        strcat(bufsend," :");
    ////                                        nbytes += strlen(clientSock[i]);
    ////                                        strcat(bufsend,buf);
    //                                        int sendsuccess = send(j,buf,nbytes,0);
    //                                        if( sendsuccess == -1){
    //                                            perror("send");
    //                                        }
    //                                    }
    //                                }
    //                            }
    //                            memset(buf,0,sizeof(buf));
    //                        }
    //                    }

    //                }
    //            }
    //        }
    //    }    // close socket

}

//-----------------------------------------------------------------------------
void* ServerChat::get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//
void ServerChat:: cleanUp(){
    close(sockfd);
}
