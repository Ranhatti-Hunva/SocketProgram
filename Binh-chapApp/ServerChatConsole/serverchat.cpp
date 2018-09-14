#include "serverchat.h"

//---------------------------------------------------------------------------------------
ServerChat::ServerChat(char* ipAddress, char* port)
    :m_ipAddr(ipAddress), m_port(port)
{

}
//---------------------------------------------------------------------------------------
ServerChat::~ServerChat(){
    cleanUp();
}

//---------------------------------------------------------------------------------------
void ServerChat::initSet(){
    memset(&hints, 0 , sizeof(hints)); // clear memory of hints
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if((rv = getaddrinfo(m_ipAddr,m_port, &hints, &servinfo)) != 0){
        std::cerr << "getaddrinfo: "<< gai_strerror(rv);
    }
}

//---------------------------------------------------------------------------------------
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

        //setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(int));
        //getsockopt();

        if(bind(sockfd, p->ai_addr, p->ai_addrlen) == -1){
            close(sockfd);
            perror("server : socket");
            exit(1);
        }

        break;
    }
    this->fdmax = sockfd;
    FD_ZERO(&read_fds);
    //FD_ZERO(&read_fds);
    FD_SET(sockfd, &read_fds);
    freeaddrinfo(servinfo);

    if (p == nullptr){
        std:: cerr << "server: failed to bind\n";
        exit(1);
    }
    return sockfd;
}
//---------------------------------------------------------------------------------------
bool ServerChat::listenSocket(int sock, int backLog){
    if(listen(sock,backLog) == -1){
        perror("listen");
        return false;
    }
    return true;
}
//---------------------------------------------------------------------------------------
int ServerChat::sendall(int socket,unsigned char *buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(socket, buf+total, std::min(bytesleft,len), 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}

//---------------------------------------------------------------------------------------

void ServerChat::sendThread( msgQueue &qSend, std::mutex &mt){
    timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 5000;
    while(1){

        if(!qSend.isEmpty()){

            mt.lock();
            fd_set sendFds;
            FD_ZERO(&sendFds);
            FD_SET(qSend.frontQ().socket,&sendFds);

            if (select(qSend.frontQ().socket+1,nullptr, &sendFds, nullptr, nullptr) <0){
                perror("Error with select on this socket");
            }

            if(FD_ISSET(qSend.frontQ().socket, &sendFds)){
                //                std::cout<<"\nlbi  --- "<<qSend.frontQ().len<<"\n";
                //                for(int i = 0; i < qSend.frontQ().len;i++){

                //                    printf("%d",qSend.frontQ().buf[i]);
                //                }

                long int numSend = sendall(qSend.frontQ().socket,
                                           qSend.frontQ().buf,
                                           qSend.frontQ().len);
                if(numSend != 0){
                    perror("send :");
                }

                qSend.popQ();


            }
            mt.unlock();
        }

    }
}

//---------------------------------------------------------------------------------------
void ServerChat::clientQRecv(struct msg_text msgHandle,
                             std::vector <clientNode> &clientList,
                             std::vector <timeoutNode> &timeoutList,
                             msgQueue &qSend, std::mutex &mt){
    ClientManage cliManage;
    //std::unique_ptr<char> msgbuf(new char[2048]);
    char *msgbuf = new char[msgHandle.msg.size()];

    strcpy(msgbuf,msgHandle.msg.c_str());
    //std::cout<<"msg :"<<msgHandle.msg<<"\n";

    //    std::cout<<"\n\nlbc  --- "<<" "<< msgHandle.msg.size()<<"\n";
    //    for(int i = 0; i < msgHandle.msg.size();i++){

    //        printf("%c",msgHandle.msg.c_str()[i]);
    //    }
    //    std::cout<<"\n\n";

    switch(msgHandle.type_msg){
    case SGI:
        //mtx.lock();
        skExist = cliManage.mapClientWithSocket(clientList,msgHandle.socketfd,msgbuf,read_fds,mt);
        if(skExist != -1){
            FD_CLR(skExist,&read_fds);
        }
        //mtx.unlock();
        //        for(int i = 0 ; i < MAX_CLIENT ; i++){
        //            if(clientList[i].status == true){
        //                std::cout<<"Name client online "<< std::string(clientList[i].name,0,20) <<" socket "<< clientList[i].socketfd<<"\n";
        //            }
        //            else if(clientList[i].id!= -1) {
        //                std::cout<<"Name client offline "<< std::string(clientList[i].name,0,20) <<"\n";
        //            }
        //        }
        cliManage.sendOffClient(qSend,mt,clientList);

        break;

    case MSG:
        //push msg will send (msg, socket)
        //mtx.lock();
        cliManage.sendMsgToClient(clientList,msgbuf,msgHandle.socketfd,timeoutList,qSend,mt);
        //mtx.unlock();
        break;
    case RSP:
        //mtx.lock();
        std::vector<timeoutNode>::iterator it;
        if(!timeoutList.empty()){
            it = std::find_if(timeoutList.begin(),timeoutList.end(),
                              [=] (timeoutNode const& f) {
                return ((f.msgID == msgHandle.ID)&&(f.socket == msgHandle.socketfd));
            });
            if (it != timeoutList.end()){
                timeoutList.erase(it);
            }
        }

        break;
    }
    delete [] msgbuf;
}
//---------------------------------------------------------------------------------------
void ServerChat::timeoutThread(std::vector <clientNode> &clientList,
                               std::vector <timeoutNode> &timeoutList){
    struct timeval tp;
    while(1){
        if(!timeoutList.empty()){

            gettimeofday(&tp, nullptr);
            if((tp.tv_sec * 1000 + tp.tv_usec / 1000) - timeoutList.front().timeout> timeOut){
                //std::cout << "co vao hehe\n";
                std::cout<<"client timeout!!! ";
                std::cout<<"socket "<<timeoutList.front().socket<<" msg ID "<<
                           timeoutList.front().msgID<<"\n";
                //close clear from fd off client;
                for(int i = 0; i<MAX_CLIENT;i++){
                    if((clientList[i].status == true)&&
                            (clientList[i].socketfd == timeoutList.front().socket)){
                        clientList[i].status = false;
                        break;
                    }
                }
                FD_CLR(timeoutList.front().socket,&read_fds);
                timeoutList.erase(timeoutList.begin());
            }
        }
    }
}

//---------------------------------------------------------------------------------------
//send respond
void clientQSend(struct msg_text msgHandle){

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
//---------------------------------------------------------------------------------------

void ServerChat::recvData(int serverFd,                          
                          std::vector<clientNode> &clientLst,
                          thread_pool &poolThread,
                          std::vector<timeoutNode> &timeoutList,
                          msgQueue &qSend,std::mutex &mt){

    while(1){


        fd_set listener = this->read_fds;
        struct timeval tv;
        tv.tv_usec = 0;
        tv.tv_sec = 1;

        //std::unique_lock<std::mutex> locker(mtx);

        //std::cout<<"fdmax "<<select(this->fdmax+1,&listener, nullptr,nullptr,&tv) << "\n";

        if(select(this->fdmax+1,&listener, nullptr,nullptr,&tv) == -1){
            perror("select");
            exit(1);
        }
        //locker.unlock();
        for(int i = 0 ; i < clientFds.size();i++){
            if(!FD_ISSET(clientFds[i],&read_fds) && skExist != -1){
                close(clientFds[i]);
                //sleep(2);
                std::cout<<"socket "<<clientFds[i]<<" closed\n";
                std::vector<int>::iterator position = std::find(clientFds.begin(), clientFds.end(), skExist);
                clientFds.erase(position);
                skExist = -1;
            }
        }
        if(FD_ISSET(serverFd,&listener)){

            addrlen = sizeof remoteaddr;
            newfd = accept(sockfd,(struct sockaddr *)&remoteaddr,&addrlen);
            if(newfd == -1){
                perror("accept");
            } else {

                //std::unique_lock<std::mutex> locker(mtx);
                FD_SET(newfd,&read_fds);
                //add socket to clientFds list
                std::vector<int>::iterator position = std::find(clientFds.begin(), clientFds.end(), newfd);
                if(position == clientFds.end()){
                    //clientFds.erase(position);
                    clientFds.push_back(newfd);
                }
                std::cout<<"size ==> "<<clientFds.size()<<"\n";
                //locker.unlock();

                long arg = fcntl(newfd, F_GETFL, NULL);
                arg |= O_NONBLOCK;
                fcntl(newfd, F_SETFL, arg);


                // assign fdmax = socket of newfd
                if( newfd > this->fdmax){
                    this->fdmax = newfd;
                }


                const char * network_name = inet_ntop(remoteaddr.ss_family,
                                                      get_in_addr((struct sockaddr*)&remoteaddr)
                                                      ,remoteIP,INET6_ADDRSTRLEN);
                std::cout <<"new connection from " << network_name <<
                            " on socket" << newfd << "\n";

            }
        }

        for(int i = 0 ; i < clientFds.size();i++){
            if(FD_ISSET(clientFds[i],&listener)){
                //std::cout<<"client id "<<clientFds[i]<<"\n";
                std::unique_ptr<uint8_t> buf (new uint8_t[16*1024]);
                //unsigned char buf[2048];

                if((nbytes = recv(clientFds[i],buf.get(),16*1024,0))<=0){


                    if ((nbytes == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK))){
                        perror("=> Message is not gotten complete !!");
                    }
                    if(nbytes == 0){
                        // close connect
                        // chuyen trang thai stt sang offline
                        for(int j = 0 ; j < MAX_CLIENT ; j++){
                            if(clientLst[j].socketfd == clientFds[i] && clientLst[j].status == true){
                                clientLst[j].status = false;
                                break;
                            }
                        }
                        std::cout << "socket "<< clientFds[i] << " closed\n";

                        close(clientFds[i]);

                        FD_CLR(clientFds[i],&read_fds);
                    }

                }
                else{


                    std::vector<uint8_t> buffer;
                    buffer.insert(buffer.end(),&buf.get()[0],&buf.get()[nbytes]);
//                    total +=nbytes;
//                    std::cout<<"byte recv =====> "<<nbytes<<"\n";
//                    std::cout<<"total recv =====> "<<total<<"\n";

                    while(buffer.size()>0){
                        msg_text recvMsg,rspMsg;
                        recvMsg.socketfd = clientFds[i];
                        bool is_success = handlMsg.unpacked_msg(recvMsg,buffer);

                        if(!is_success){
                            break;
                        }
                        else{

                            //enQ thread pool
                            if(recvMsg.type_msg != RSP){

                                rspMsg.ID = recvMsg.ID;
                                rspMsg.type_msg = RSP;
                                rspMsg.socketfd = clientFds[i];

                                poolThread.enqueue([rspMsg]{
                                    clientQSend(rspMsg);
                                });
                            }

                            poolThread.enqueue([=,&clientLst,&timeoutList,&qSend,&mt]{
                                clientQRecv(recvMsg,clientLst,timeoutList,qSend,mt);
                            });

                        }
                    }//end while

                }
            }
        }
    }
}
//---------------------------------------------------------------------------------------
// xu li Qrecv

//void ServerChat::handlQrecv(std::vector <nodeRecv> &dataRecv){
//    if(!dataRecv.empty()){

//        for( std::vector< nodeRecv >::const_iterator b = dataRecv.begin(), e = dataRecv.end(); b != e; ++b )
//        {
//            std::vector<unsigned char> buffer;
//            buffer.insert(buffer.end(),b.base()->data.begin(),b.base()->data.end());
//            while(buffer.size()>0){


//                msg_text recvMsg,rspMsg;
//                recvMsg.socketfd = b.base()->socket;
//                bool is_success = handlMsg.unpacked_msg(recvMsg,buffer);

//                if(!is_success){
//                    break;
//                }
//                else{

//                    //enQ thread pool
//                    if(recvMsg.type_msg != RSP){

//                        rspMsg.ID = recvMsg.ID;
//                        rspMsg.type_msg = RSP;
//                        rspMsg.socketfd = b.base()->socket;
//                        //std::cout <<"=>------- "<<rspMsg.ID<<"\n";
//                        //poolThread.enqueue([rspMsg]{
//                            clientQSend(rspMsg);
//                        //});
//                    }

//                    //poolThread.enqueue([=,&clientLst,&timeoutList,&qSend,&mt]{
//                    //    clientQRecv(recvMsg,clientLst,timeoutList,qSend,mt);
//                    //});

//                }
//            }//end while

////            std::cout <<"\nsk => "<< b.base()->socket << "\n";
////            std::cout<<"size => "<< b.base()->data.size() << "\n";

//        }
//    }
//}



//---------------------------------------------------------------------------------------
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


    //FD_ZERO(&listener);
    FD_ZERO(&read_fds);
    //FD_SET(sockfd, &listener);

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


    // thread timeout
    //    pool.enqueue([&]{
    //        timeoutThread(listener,client,timeoutList);
    //    });
    //thread main
    while(1){
        //read_fds = listener;

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
                        //FD_SET(newfd,&listener);

                        // assign fdmax = socket of newfd
                        if( newfd > fdmax){
                            fdmax = newfd;
                        }
                        const char * network_name = inet_ntop(remoteaddr.ss_family,
                                                              get_in_addr((struct sockaddr*)&remoteaddr)
                                                              ,remoteIP,INET6_ADDRSTRLEN);
                        std::cout <<"\nnew connection from " << network_name <<
                                    " on socket" << newfd << "\n";

                    }
                } else {
                    // handling data from client
                    // khong nhan duoc data tu clien -> dong ket noi client
                    std::unique_ptr<unsigned char> buf (new unsigned char[4096]);
                    //unsigned char *buf = new unsigned char[2048];
                    memset(buf.get(),0,4096);
                    if((nbytes = recv(i,buf.get(),4096,0))<=0){
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
                        //FD_CLR(i,&listener);

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

                        std::vector<unsigned char> buffer;
                        buffer.insert(buffer.end(),&buf.get()[0],&buf.get()[nbytes]);
                        //std::cout <<"nhan " << nbytes <<"\n";

                        while(buffer.size()>0){
                            msg_text recvMsg,rspMsg;
                            recvMsg.socketfd = i;
                            bool is_success = handlMsg.unpacked_msg(recvMsg,buffer);

                            if(!is_success){
                                break;
                            }
                            else{

                                if(recvMsg.type_msg == SGI){
                                    rspMsg.ID = recvMsg.ID;

                                    //std::cout<<"msg id :"<<recvMsg.ID<<"\n";

                                    rspMsg.type_msg = RSP;
                                    rspMsg.msg = "";
                                    rspMsg.socketfd = i;
                                    clientQSend(rspMsg);
                                    usleep(1000);//1ms
                                    //clientQRecv(recvMsg,client,timeoutList);
                                    if(skExist != -1){
                                        mtx.lock();
                                        close(skExist);
                                        //  FD_CLR(skExist,&listener);
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
                        }//end while
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

            pool.enqueue([&,qMsg]{
                // clientQRecv(qMsg,client,timeoutList);
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
                // clientQSend(qMsg);
            });
            qSend.pop();
        }

    }    // end while -close socket

}

//---------------------------------------------------------------------------------------
void* ServerChat::get_in_addr(struct sockaddr *sa){
    if(sa->sa_family == AF_INET){
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
//---------------------------------------------------------------------------------------
void ServerChat:: cleanUp(){
    close(sockfd);
}
