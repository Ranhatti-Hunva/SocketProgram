#include "clientchat.h"
#include "handlemsg.h"

void recvMsg(char buf[],int sockfd){
    while(1){
        memset(buf,0,sizeof(buf));
        int bytesRecv = recv(sockfd,buf,1024,0);
        if(bytesRecv >0){
            std::cout << "> " << std::string(buf,0,bytesRecv) << std::endl;
        }
    }
}

int main()
{
    struct msg_text msgSend;
    msgSend.type_msg = SGI;
    ClientChat client;
    char buf[1024];
    std::string userInput;
    //    std::cout << "IP address: ";
    std::cout << "Your name: ";
    std::string name;
    //getline(std::cin,name);
    std::cin >> name;
    std::cout <<" name la: " <<name << "\n";
    msgSend.msg.assign(name);
    unsigned char buffer[msgSend.msg.length()+9];
    HandleMsg handleMsg;

    handleMsg.packed_msg(msgSend,buffer);
    //    std::cout << "\n=> Msg: " << msgSend.msg << std::endl;
    //    std::cout << "=> Buffer in byte (unsigned char): ";
    //    for(unsigned int i=0; i<msgSend.msg.length()+9; i++)
    //    {
    //        std::cout << (int)buffer[i] << " ";
    //    }
    //    std::cout << "\n=> ID msg: " << msgSend.ID << std::endl << std::endl;


    int socket = client.timeoutConnect("localhost","8096",10);


    if(socket > 0){
        std::cout << " socket" <<socket<<"\n";
        std::cout << "bufferbf: "<< std::endl;
        for(unsigned int i=0; i<msgSend.msg.length()+9; i++)
        {
            std::cout << (int)buffer[i] << " ";
        }
        int num = send(socket,buffer,sizeof(buffer),0);
        std::cout << "buffer: "<<num << std::endl;
        for(unsigned int i=0; i<msgSend.msg.length()+9; i++)
        {
            std::cout << (int)buffer[i] << " ";
        }
        int numRecv = recv(socket,buf,sizeof(buf),0);
        if( numRecv >=0){
            std::cout << "client connecting to" << client.getServerName() <<"\n";
            std::cout << "start chat now" <<"\n";
            //sleep(1);
        }
        fcntl(socket, F_SETFL, O_NONBLOCK);
        std::thread recvThread(recvMsg,buf,socket);
        while(1){
            std::cout  << ">";
            std:: cin >> userInput;
            //getline(std::cin,userInput);
            if(userInput.size() >0){
                int sendResult = client.sendall(socket,userInput.c_str(),userInput.size());
                if(sendResult == -1){
                    perror("send");
                    exit(1);
                }
            }
            else{
                break;
            }
        }
    }





    //        do{
    //            std::cout  << ">";
    //            getline(std::cin,userInput);

    //            //std::cout  << "max size " << userInput.max_size()<<"\n";
    //            if(userInput.size() >0){
    //                int sendResult = client.sendall(socket,userInput.c_str(),userInput.size());
    //                if(sendResult == -1){
    //                    perror("send");
    //                    exit(1);
    //                }
    //            }

    //        } while(userInput.size()>0);
    //while(1){}
    //}



    //    client.initClient();
    //    client.createSocket();
    //    client.mainLoop();
    //    client.cleanUp();
    return 0;
}
