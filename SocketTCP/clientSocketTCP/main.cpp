#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <vector>
#include <fcntl.h>

#include "usercommand.h"
#include "iosocket.h"
#include "TCPhelper.h"

using namespace std;
string user_name;

class scoped_thread
{
    std::thread t;
public:
    explicit scoped_thread(std::thread t_):t(std::move(t_))
    {
        if (!t.joinable())
        {
            throw std::logic_error("No thread");
        }
    }
    ~scoped_thread()
    {
        t.join();
    }
    scoped_thread(scoped_thread const&)=delete;
    scoped_thread& operator = (scoped_thread const&) = delete;
};

int main()
{
    /* Notice the program information */
    printf("\n-------------**--------------\n\n");
    TCPclient client_helper;

    /*------------------------------------------------------------------------------------------------------------*/
    // Searching server information.
    struct addrinfo *server_infor, *p;
    server_infor = client_helper.get_addinfo_list("",1500);
    p = server_infor;

    /*------------------------------------------------------------------------------------------------------------*/
    // Start program.
    bool is_finsh = false;
    while (!is_finsh)
    {
        // User login.
        printf("=> Please inser user's names before start program:");
        getline(cin, user_name);
        if(user_name.empty())
        {
            printf("=> Sorry!! Login fail....");
            exit(EXIT_FAILURE);
        };

        // Connection with timeout to server.
        int client_fd;
        while((client_fd=client_helper.connect_with_timeout(p))<0)
        {
            int flage_reconnect = is_reconnect(client_fd);
            if (flage_reconnect < 0)
            {
                exit(EXIT_FAILURE);
            }
        }

        /*------------------------------------------------------------------------------------------------------------*/
        // Connection success and start to communication.
        printf("\n=> Enter # to end the connection \n\n");

        long status;

        user_command user_command;   // object to project race-conditon on user input from terminal.
        user_command.clear();        // search properties on usercomnand.h

        fd_set read_fds;
        FD_ZERO(&read_fds);

        fd_set master;
        FD_ZERO(&master);
        FD_SET(client_fd, &master);

        scoped_thread sendThread(std::thread(send_TCP,std::ref(user_command), std::ref(master), std::ref(client_fd)));
        const unsigned int recv_bufsize = 256;
        char recv_buffer[recv_bufsize];

        while(!user_command.compare("#"))
        {
            struct timeval recv_tv;
            recv_tv.tv_sec = 0;
            recv_tv.tv_usec = 5000;

            read_fds = master;
            if (select(client_fd+1,&read_fds, nullptr, nullptr, &recv_tv) <0)
            {
                perror("=> Select");
                exit(EXIT_FAILURE);
            }

            if (FD_ISSET(client_fd, &read_fds))
            {
                memset(&recv_buffer,0,recv_bufsize);
                if ((status=recv(client_fd,recv_buffer,recv_bufsize,0)) <=0 )
                {
                    if ((status == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                    {
                        perror("=> Message is not gotten complete !!");
                    }
                    else
                    {
                        if (status == 0)
                        {
                            printf("=> Connection has been close\n");
                        }
                        else
                        {
                            printf("=> Error socket.");
                        };
                        user_command.set("#");
                        break;
                    };
                }
                else
                {
                    cout << "Messaga from server :" << recv_buffer << endl;
                };
            };

//            int is_msg_recived = client_helper.recv_msg(client_fd);
//            if (is_msg_recived == -1)
//            {
//                user_command.set("#");
//            }
//            else if (is_msg_recived == 1)
//            {
//                cout << "Messaga from server :" << client_helper.msg_incomplete << endl;
//            };
        };
        is_finsh = is_reconnect(client_fd);
    };

    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    freeaddrinfo(server_infor);
    printf("\n=> Socket is closed.\n=> Goodbye...\n");
    return 0;
}
