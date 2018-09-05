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
            perror("server : socket");
            continue;
        }
        //check reuse addr
        if(setsockopt(sockfd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1){
            perror("setsockopt");
            exit(1);
        }

        setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));
        //getsockopt();

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server : socket");
            exit(1);
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
void ServerChat::clientQRecv(struct msg_text msgHandle,
                             std::vector <clientNode> &clientList,
                             std::vector <timeoutNode> &timeoutList){
    ClientManage cliManage;

    std::unique_ptr<char> msgbuf(new char[msgHandle.msg.size()]);

    //char *msgbuf = new char[msgHandle.msg.size()];


    strcpy(msgbuf.get(),msgHandle.msg.c_str());
    //std::cout<<"msg :"<<msgHandle.msg<<"\n";

    switch(msgHandle.type_msg){
    case SGI:
        skExist = cliManage.mapClientWithSocket(clientList,msgHandle.socketfd,msgbuf.get(),listener);

        for(int i = 0 ; i < MAX_CLIENT ; i++){
            if(clientList[i].status == true){
                std::cout<<"Name client online "<< std::string(clientList[i].name,0,20) <<" socket "<< clientList[i].socketfd<<"\n";
            }
            else if(clientList[i].id!= -1) {
                std::cout<<"Name client offline "<< std::string(clientList[i].name,0,20) <<"\n";
            }
        }
        break;

    case MSG:
        cliManage.sendMsgToClient(clientList,msgbuf.get(),msgHandle.socketfd,timeoutList);
        //add Q time out wait for rsp
        //std::cout<<"id msg bf send: " <<msgHandle.ID<< " from socket "<<msgHandle.socketfd <<"\n";
        break;
    case RSP:
        std::vector<timeoutNode>::iterator it;
        if(!timeoutList.empty()){
            it = std::find_if(timeoutList.begin(),timeoutList.end(),
                              [=] (timeoutNode const& f) {
                return ((f.msgID == msgHandle.ID)&&(f.socket == msgHandle.socketfd));
            });
            if (it != timeoutList.end()){
                timeoutList.erase(it);
            }
//            std::cout<<timeoutList.front().msgID<<" "<<
//                       timeoutList.front().socket<<" "<<
//                        timeoutList.front().timeout<<"\n";
//            std::cout <<"hehehe \n";
        }
        std::cout<<"rsp id msg: " <<msgHandle.ID<< " from socket "<<msgHandle.socketfd <<"\n";
        //find and erase msg in timeout list
        break;
    }
    //delete [] msgbuf;
}
// thread time for time out ---------------------------------------------------
void ServerChat::timeoutThread(fd_set &fd,
                               std::vector <clientNode> &clientList,
                               std::vector <timeoutNode> &timeoutList){
    struct timeval tp;
    while(1){
        if(!timeoutList.empty()){

            gettimeofday(&tp, nullptr);
            if((tp.tv_sec * 1000 + tp.tv_usec / 1000) - timeoutList.front().timeout> timeOut){
                //std::cout << "co vao hehe\n";
                std::cout<<"client timeout!!!\n";
                //close clear from fd off client;
                for(int i = 0; i<MAX_CLIENT;i++){
                    if((clientList[i].status == true)&&
                            (clientList[i].socketfd == timeoutList.front().socket)){
                        clientList[i].status = false;
                        break;
                    }
                }
                FD_CLR(timeoutList.front().socket,&fd);
                timeoutList.erase(timeoutList.begin());
            }
        }
    }
}

//-----------------------------------------------------------------------------
//send respond
void clientQSend(struct msg_text msgHandle/*, std::vector <clientNode> &clientList*/){

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
// in loop:  client connect check profile, has client -> show name - status on
//           hasn't client yet -> add profile -> show name -status on
//           check profile client has msg recv -> send msg to client
//           client disconnect -> show name - status off
//           client chat all -> broadcast msg to all client online
//           client chat personal -> send msg if online else store msg in file


void ServerChat::mainLoop(){


    std::cout<<"Server started!!!\n";

    std::vector<timeoutNode> timeoutList;


    FD_ZERO(&listener);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &listener);

    fdmax = sockfd;
    int flag = 0;

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
    // thread timeout
    pool.enqueue([&]{
        timeoutThread(listener,client,timeoutList);
    });
    //thread main
    while(1){
        read_fds = listener;

        if(select(fdmax+1,&read_fds, nullptr,nullptr,nullptr) == -1){
            perror("select");
            exit(1);
        }


        for(int i = sockfd; i<= fdmax; i++){

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
                        std::cout <<"\nnew connection from " << network_name <<
                                    " on socket" << newfd << "\n";

                        clientSocket = newfd;
                    }
                } else {
                    // handling data from client
                    // khong nhan duoc data tu clien -> dong ket noi client
                    std::unique_ptr<unsigned char> buf (new unsigned char[2048]);
                    //unsigned char *buf = new unsigned char[2048];
                    memset(buf.get(),0,2048);
                    if((nbytes = recv(i,buf.get(),2048,0))<=0){
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
                        struct msg_text recvMsg,rspMsg;
                        recvMsg.ID = 0;
                        recvMsg.msg ="";
                        recvMsg.type_msg = -1;
                        recvMsg.socketfd = i;
//                        std::cout<<"recv \n";
                        bool is_success = handlMsg.unpacked_msg(recvMsg,buf.get(),nbytes);
//                        for(int i = 0; i< nbytes;i++){
//                            std::cout<<(unsigned int)buf.get()[i]<<" ";
//                        }
//                        std::cout<<"\n";
//                        //std::cout<<(unsigned int)buf.get()<<"\n";
//                        if(is_success){
//                            std::cout<<"id "<<recvMsg.ID<<" type "<<(int)recvMsg.type_msg
//                                    <<" content "<<recvMsg.msg<<"\n";
//                        }

                        if(recvMsg.type_msg == SGI){
                            rspMsg.ID = recvMsg.ID;

                            //std::cout<<"msg id :"<<recvMsg.ID<<"\n";

                            rspMsg.type_msg = RSP;
                            rspMsg.msg = "";
                            rspMsg.socketfd = i;
                            clientQSend(rspMsg);
                            usleep(1000);//1ms
                            clientQRecv(recvMsg,client,timeoutList);
                            if(skExist != -1){
                                mtx.lock();
                                close(skExist);                                
                                FD_CLR(skExist,&listener);
                                skExist = -1;
                                mtx.unlock();

                                //send(skExist,"close",5,0);
                                //close(skExist);
                            }

                        }

                        //qSend send respond to client
                        //qRecv send msg to another client

                        if(recvMsg.type_msg != RSP && recvMsg.type_msg != SGI){
                            rspMsg.ID = recvMsg.ID;

                            //std::cout<<"msg id :"<<recvMsg.ID<<"\n";

                            rspMsg.type_msg = RSP;
                            rspMsg.msg = "";
                            rspMsg.socketfd = i;
                            qSend.push(rspMsg);
                        }
                        if(recvMsg.type_msg != SGI){
                            qRecv.push(recvMsg);
                        }

                        //delete[] buf;
                    }
                }
            }
        }// end for
        while(!qRecv.empty()){
            struct msg_text qMsg;
            qMsg.ID = qRecv.front().ID;
            qMsg.msg = qRecv.front().msg;
            qMsg.type_msg = qRecv.front().type_msg;
            qMsg.socketfd = qRecv.front().socketfd;

            pool.enqueue([&]{
                clientQRecv(qMsg,client,timeoutList);
            });
            qRecv.pop();
        }
        while(!qSend.empty()){
            struct msg_text qMsg;
            qMsg.ID = qSend.front().ID;
            qMsg.msg = qSend.front().msg;
            qMsg.type_msg = qSend.front().type_msg;
            qMsg.socketfd = qSend.front().socketfd;
            // sleep(6); //test timeout
            pool.enqueue([qMsg]{
                clientQSend(qMsg);
            });
            qSend.pop();
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
