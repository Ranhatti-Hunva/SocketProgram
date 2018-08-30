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
void clientQRecv(struct msg_text msgHandle, std::vector <clientNode> &clientList){
    ClientManage cliManage;
    char *msgbuf = new char(msgHandle.msg.size());
    strcpy(msgbuf,msgHandle.msg.c_str());
    std::cout<<"\n msg :"<<msgHandle.msg<<"\n";
    switch(msgHandle.type_msg){
    case SGI:
        cliManage.mapClientWithSocket(clientList,msgHandle.socketfd,msgbuf);
        for(int i = 0 ; i < MAX_CLIENT ; i++){
            if(clientList[i].status == true){
                std::cout<<"name client online "<< std::string(clientList[i].name,0,20) <<" socket "<< clientList[i].socketfd<<"\n";
            }
            else if(clientList[i].id!= -1) {
                std::cout<<"name client offline "<< std::string(clientList[i].name,0,20) <<"\n";
            }
        }
        break;
    case MSG:
        cliManage.sendMsgToClient(clientList,msgbuf,msgHandle.socketfd);
        break;
    default:
        break;
    }
}

//-----------------------------------------------------------------------------
void clientQSend(struct msg_text msgHandle, std::vector <clientNode> &clientList){
//    ClientManage cliManage;
//    char *msgbuf = new char(msgHandle.msg.size());
//    strcpy(msgbuf,msgHandle.msg.c_str());

    HandleMsg grapMsg;


    switch(msgHandle.type_msg){
    case RSP:
        unsigned char buffer[9];
        grapMsg.packed_msg(msgHandle,buffer);
        send(msgHandle.socketfd,buffer,9,0);
        break;
    default:
        break;
    }
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

    FD_ZERO(&listener);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &listener);


    fdmax = sockfd;
    int flag = 0;
//    int clientOnline = 0;
//    std::map<int,char*> clientSock;
    fcntl(sockfd, F_SETFL, O_NONBLOCK);

    // tao list client
    std::vector<clientNode> client(MAX_CLIENT);
    for(int i = 0 ; i < MAX_CLIENT ; i++){
        client[i].name = "";
        client[i].status = false;
        client[i].socketfd = -1;
        client[i].id = -1;
    }
    //clientNode clientTemp[1];
    int clientSocket = -1;


    //std::cout << "number = 5\n";




    while(1){
        read_fds = listener;
        if(select(fdmax+1,&read_fds, nullptr,nullptr,nullptr) == -1){
            perror("select");
            exit(1);
        }
        for(int i = 0; i<= fdmax; i++){
            if(FD_ISSET(i,&read_fds)){
                //server listening connection
                if(i == sockfd){
                    addrlen = sizeof remoteaddr;
                    newfd = accept(sockfd,(struct sockaddr *)&remoteaddr,&addrlen);
                    if(newfd == -1){
                        perror("accept");
                    } else {

                        flag = 1;
                        FD_SET(newfd,&listener);
                        // assign fdmax = socket of newfd
                        if( newfd > fdmax){
                            fdmax = newfd;
                        }
                        const char * network_name = inet_ntop(remoteaddr.ss_family,
                                                              get_in_addr((struct sockaddr*)&remoteaddr)
                                                              ,remoteIP,INET6_ADDRSTRLEN);
                        std::cout <<"new connection from" << network_name <<
                                    " on socket" << newfd << "\n";

                        clientSocket = newfd;
                    }
                } else {
                    // handling data from client
                    // khong nhan duoc data tu clien -> dong ket noi client

                    if((nbytes = recv(i,buf,4096,0))<=0){
                        if(nbytes == 0){
                            // close connect
                            // chuyen trang thai stt sang offline
                            for(int j = 0 ; j < MAX_CLIENT ; j++){
                                if(client[j].socketfd == i && client[j].status == true){
                                    client[j].status = false;
                                    break;
                                }
                            }
                            std::cout << "socket "<< i << " closed\n";
                        } else {
                            perror("revc");
                        }
                        close(i);
                        FD_CLR(i,&listener);
                    }

                    //nhan duoc data -> xu li data
                    else {
                        //std::cout<<sizeof(buf)<<std::endl;

                        // add data vao Qrecv
                        /* tach data thanh cac truong length - id destination - data
                            check client online -> xu li du lieu
                            offline -> luu vao array client
                        */

                        // xuli du lieu
                        /* lay name client -> gan vao data -> dia chi dich
                           -> chuyen qua Qsend
                        */

                        // add data vao Qsend
                        /* tach data = data + dia chi dich
                           xac dinh dia chi dich
                           truyen tung nguoi hoac truyen tat ca
                           send -> deQmsg
                        */

                        // truyen tung nguoi

                        // truyen tat ca
                        std::ostringstream  number_str;

                        number_str << i;

                        //strcat(buf,(number_str.str()).c_str());
                        struct msg_text recvMsg,rspMsg;
                        recvMsg.ID = 0;
                        recvMsg.msg ="";
                        recvMsg.type_msg = -1;
                        recvMsg.socketfd = i;

                        bool is_success = handlMsg.unpacked_msg(recvMsg,buf,nbytes);

                        rspMsg.ID = recvMsg.ID;

                        std::cout<<"msg id :"<<recvMsg.ID<<"\n";

                        rspMsg.type_msg = RSP;
                        rspMsg.msg = "";
                        rspMsg.socketfd = i;
                        qRecv.push(recvMsg);
                        qSend.push(rspMsg);

                        memset(buf,0,4096);
                    }
                }
            }
        }// end for
        if(!qRecv.empty()){
            struct msg_text qMsg;
            qMsg.ID = qRecv.front().ID;
            qMsg.msg = qRecv.front().msg;
            qMsg.type_msg = qRecv.front().type_msg;
            qMsg.socketfd = qRecv.front().socketfd;

            pool.enqueue([qMsg,&client]{
                clientQRecv(qMsg,client);
            });
            qRecv.pop();
        }



    }    // end while -close socket

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
