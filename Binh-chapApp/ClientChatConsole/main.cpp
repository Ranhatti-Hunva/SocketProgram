#include "clientchat.h"
#include "handlemsg.h"

void recvMsg(char buf[],int sockfd){
    while(1){
        memset(buf,0,sizeof(buf));
        int bytesRecv = recv(sockfd,buf,1024,0);
        if(bytesRecv >0){
            std::cout << "> " << std::string(buf,0,bytesRecv) << std::endl;
        }
        if(bytesRecv == 0){
            std::cout << "> server is close \n";
            close(sockfd);
            exit(1);
        }
    }
}

int main()
{
    struct msg_text msgSend;
    msgSend.type_msg = SGI;
    ClientChat client;
    unsigned char * buf = new unsigned char [1024];
    char bufrcv[1024];
    char statusBuf[10];
    std::string userInput;
    //    std::cout << "IP address: ";
    std::cout << "Your name: ";
    std::string name;
    getline(std::cin,name);
    msgSend.msg.assign(name);
    unsigned char buffer[msgSend.msg.length()+9];
    HandleMsg handleMsg;

    handleMsg.packed_msg(msgSend,buffer);


    int socket = client.timeoutConnect("localhost","8096",10);


    if(socket > 0){

        int num = send(socket,buffer,sizeof(buffer),0);


        while(1){
            int numRecv = recv(socket,statusBuf,10,0);

            if(strcmp(statusBuf,"success") == 0){
                std::cout << "Login success\n";
                std::cout << "Start chat now" <<"\n";
                std::cout  << "press '#' to exit\n";
                break;
            }
            if(strcmp(statusBuf,"existed") == 0){
                std::cout << "Username is existed please use another name\n";

                //std::cin.ignore();
                getline(std::cin,name);
                msgSend.msg.clear();
                msgSend.msg.assign(name);
                unsigned char nameResend[msgSend.msg.length()+9];
                handleMsg.packed_msg(msgSend,nameResend);
                send(socket,nameResend,sizeof(nameResend),0);
            }
            if(strcmp(statusBuf,"full") == 0){
                std::cout << "server is full !!! can't login server\n";
                close(socket);
                return 1;
            }
            memset(statusBuf,0,10);
            //nhan msg hop le

        }


        fcntl(socket, F_SETFL, O_NONBLOCK);

        //struct msg_text msgSend;

        std::thread recvThread(recvMsg,bufrcv,socket);
        while(1){
            std::cout  << ">";
            //std:: cin >> userInput;
            getline(std::cin,userInput);
            if(strcmp(userInput.c_str(),"#") == 0){
                close(socket);
                exit(1);
            }
            if(userInput.size() >0){
                msgSend.type_msg = MSG;
                msgSend.msg.assign(userInput);
                unsigned char bufSend[userInput.size()+9];
                memset(bufSend,0,userInput.size()+9);
                handleMsg.packed_msg(msgSend,bufSend);
                int sendResult = send(socket,bufSend,sizeof(bufSend),0);
                if(sendResult == -1){
                    perror("send");
                    exit(1);
                }
            }
//            else if(userInput.size() == 0){
//                break;
//            }
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
