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

#include "msgqueue.h"
#include "iosocket.h"
#include "tcphelper.h"
#include "threadpool.h"

using namespace std;
string user_name;
bool is_error;

int main()
{
    /* Notice the program information */
    printf("\n              -------------**--------------               \n\n");
    TCPclient client_helper; // Help client for any TCP process needed.

    /*------------------------------------------------------------------------------------------------------------*/
    // Searching server information. Change "" to specific host name if use remote server.
    struct addrinfo *server_infor, *p;
//    server_infor = client_helper.get_addinfo_list("10.42.0.188",1500);
    server_infor = client_helper.get_addinfo_list("10.42.0.127",8096);
//    server_infor = client_helper.get_addinfo_list("",1500);
    p = server_infor;

    /*------------------------------------------------------------------------------------------------------------*/
    // User login.
    user_name = "All";
    printf("=> Please inser user's names before start program:");
    while(!user_name.compare("All"))
    {
        getline(cin, user_name);

        if(user_name.empty())
        {
            printf("=> Sorry!! Login fail.... \n");
            exit(EXIT_FAILURE);
        };

        if(!user_name.compare("All"))
        {
            printf("=> Sorry!! You can't use this name. Plz re-insert another username:");
        };
    };

    // Start program.
    bool end_connection = false;
    is_error = false;
    while (!end_connection)
    {
        // Thread pool
        thread_pool threads(10);

        // Queue for the massages are wating to send.
        msg_queue msg_wts;
        msg_wts.clear(Q_MSG);
        msg_wts.clear(Q_RSP);

        int client_fd;

        // read terminal
        threads.enqueue(read_terminal, ref(end_connection), ref(client_helper), ref(msg_wts), ref(threads));

        // Connection with timeout to server.
        while(!end_connection)
        {
            client_fd = client_helper.connect_with_timeout(p);
            if(client_fd < 0)
            {
                if(end_connection)
                {
                    exit(EXIT_FAILURE);
                }
                else
                {
                    printf("=> Reconnection again! \n");
                    close(client_fd);
                    sleep(1);
                    continue;
                };
            }
            else
            {
                printf("=> Connect succesffull! \n");
                printf("=> Welcome!! Enter # to end the connection \n\n");
                break;
            }
        };

        /*------------------------------------------------------------------------------------------------------------*/
        // Connection success and start to communication.

        threads.enqueue([&]()
        {
            client_helper.send_msg(msg_wts, end_connection, client_fd);
        });

        threads.enqueue([&]()
        {
            client_helper.timeout_clocker(end_connection);
        });

        // Listern incomming message
        while(!end_connection)
        {
            int is_disconnect = client_helper.recv_msg(client_fd, msg_wts, threads);
            if(is_disconnect < 0)
            {
                end_connection = true;
                is_error = true;
                sleep(2);
                break;
            }
        };

        if (is_error)
        {
            // Restart the process beciuse
            end_connection = false;
            is_error = false;
            close(client_fd);
        }
        else
        {
            // Does user really want to disconnect?
            end_connection = is_reconnect(client_fd);
        };
    };

    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    freeaddrinfo(server_infor);
    printf("=> Socket is closed.\n=> Goodbye...\n");
    return 0;
}
