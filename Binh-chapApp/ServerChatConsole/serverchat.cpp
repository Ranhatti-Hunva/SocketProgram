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
// wait connect
// close listening socket
// loop
// in loop:  client connect check db, has client -> show name - status on
//           hasn't client yet -> add db -> show name -status on
//           check db client has msg recv -> send msg to client
//           client disconnect -> show name - status off
//           client chat all -> broadcast msg to all client online
//           client chat personal -> send msg if online else store msg in db

void ServerChat::mainLoop(){

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    FD_SET(sockfd, &master);
    fdmax = sockfd;
    int flag = 0;
    int clientOnline = 0;
    std::map<int,char*> clientSock;
    while(1){
        read_fds = master;
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
                        clientOnline++;
                        flag = 1;
                        FD_SET(newfd,&master);
                        // assign fdmax = socket of newfd
                        if( newfd > fdmax){
                            fdmax = newfd;
                        }
                        const char * network_name = inet_ntop(remoteaddr.ss_family,
                                                              get_in_addr((struct sockaddr*)&remoteaddr)
                                                              ,remoteIP,INET6_ADDRSTRLEN);
                        std::cout <<"new connection from" << network_name <<
                                    " on socket" << newfd << "\n";
                    }
                } else {
                    // handling data from client
                    if(flag == 1){
                        int numRecv = recv(i,buf,sizeof(buf),0);
                        if(numRecv > 0){
                            std::cout <<"client " << std::string(buf,0,numRecv) <<
                                        " online\n";
                            clientSock[i] = buf;
                            send(i,"ACK",4,0);
                        }
                        flag = 0;
                        break;
                    }
                  else{
                        if((nbytes = recv(i,buf,sizeof(buf),0))<=0){
                            if(nbytes == 0){
                                // close connect
                                std::cout << "socket "<< i << " closed\n";
                            } else {
                                perror("revc");
                            }
                            close(i);
                            FD_CLR(i,&master);
                        } else {

                            //std::cout<<sizeof(buf)<<std::endl;
                            for(int j = 0; j <= fdmax; j++){
                                if (FD_ISSET(j, &master)){
                                    if(j!= sockfd && j != i){

                                        std:: cout << j <<" ooooOOOoooo"<< std::endl;
//                                        std::cout<< nbytes << std::endl;
//                                        char bufsend[4096];
//                                        strcpy(bufsend,clientSock[i]);
//                                        strcat(bufsend," :");
//                                        nbytes += strlen(clientSock[i]);
//                                        strcat(bufsend,buf);
                                        int sendsuccess = send(j,buf,nbytes,0);
                                        if( sendsuccess == -1){
                                            perror("send");
                                        }
                                    }
                                }
                            }
                            memset(buf,0,sizeof(buf));
                        }
                    }

                }
            }
        }
    }    // close socket
    cleanUp();
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
