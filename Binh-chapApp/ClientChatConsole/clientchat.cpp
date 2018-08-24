#include "clientchat.h"

ClientChat::ClientChat(char * svAddr, char * port)
    :m_svAddr(svAddr),m_port(port)
{
}

ClientChat::~ClientChat()
{
    cleanUp();
}

void * ClientChat::get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}



void recvMsg(char buf[],int sockfd){
    while(1){
        memset(buf,0,sizeof(buf));
        int bytesRecv = recv(sockfd,buf,4096,0);
        if(bytesRecv >0){
            std::cout << "SERVER> " << std::string(buf,0,bytesRecv) << std::endl;
        }
    }
}

void ClientChat::initClient(){
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(m_svAddr, m_port, &hints, &servinfo)) != 0) {
        std::cerr<<"getaddrinfo: " << gai_strerror(rv) << "\n";
        exit(1);
    }
}


int ClientChat::createSocket(){
    for( p = servinfo; p!= nullptr; p = p->ai_next){
        sockfd = socket(p->ai_family,p->ai_socktype,p->ai_protocol);
        if(sockfd == -1){
            perror("client: socket");
            continue;
        }

        if(connect(sockfd,p->ai_addr,p->ai_addrlen) == -1){
            close(sockfd);
            perror("client: connect");
            exit(1);
        }

        break;
    }

    if(p == nullptr){
        std :: cerr << "client failed to connect";
        return 2;
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr),
              s, sizeof s);

    freeaddrinfo(servinfo);
//    std::cout << "Your name: ";
//    char name[20];
//    std::cin >> name;
//    send(sockfd,name,strlen(name),0);
//    int numRecv = recv(sockfd,buf,sizeof(buf),0);
//    if( numRecv >=0){
         std::cout << "client connecting to" << s <<"\n";
//         std::cout << "start chat now" <<"\n";
//         sleep(10);
//    }
    return sockfd;
}


int ClientChat::sendall(int socket,const char *buf, int len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = len; // how many we have left to send
    int n;

    while(total < len) {
        n = send(socket, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }

    len = total; // return number actually sent here

    return n==-1?-1:0; // return -1 on failure, 0 on success
}



void ClientChat::mainLoop(){

    std::string userInput;
    std::cout << "Your name: ";
    std::string name;
    getline(std::cin,name);
    send(sockfd,name.c_str(),name.size()+1,0);
    int numRecv = recv(sockfd,buf,sizeof(buf),0);
    if( numRecv >=0){
         std::cout << "client connecting to" << s <<"\n";
         std::cout << "start chat now" <<"\n";
         //sleep(1);
    }
    fcntl(sockfd, F_SETFL, O_NONBLOCK);
    std::thread first(recvMsg,buf,sockfd);
    do{
        std::cout  << ">";
        getline(std::cin,userInput);

        //std::cout  << "max size " << userInput.max_size()<<"\n";
        if(userInput.size() >0){
            int sendResult = sendall(sockfd,userInput.c_str(),userInput.size());
            if(sendResult == -1){
                perror("send");
                exit(1);
            }
        }

    } while(userInput.size()>0);
}



void ClientChat::cleanUp(){
    close(sockfd);
}
