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

using namespace std;
string user_name;

// RAII for mutil thread.
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
    TCPclient client_helper; // Help client for any TCP process needed.

    /*------------------------------------------------------------------------------------------------------------*/
    // Searching server information. Change "" to specific host name if use remote server.
    struct addrinfo *server_infor, *p;
    server_infor = client_helper.get_addinfo_list("",1500);
    p = server_infor;

    /*------------------------------------------------------------------------------------------------------------*/
    // User login.
    printf("=> Please inser user's names before start program:");
    getline(cin, user_name);
    if(user_name.empty())
    {
        printf("=> Sorry!! Login fail.... \n");
        exit(EXIT_FAILURE);
    };

    // Start program.
    bool is_finsh = false;
    while (!is_finsh)
    {
        // Connection with timeout to server.
        int client_fd;
        while((client_fd=client_helper.connect_with_timeout(p))<0)
        {
            // Error connect to server or connect timeout.
            int flage_reconnect = is_reconnect(client_fd);
            if (flage_reconnect < 0)
            {
                exit(EXIT_FAILURE);
            }
        }

        /*------------------------------------------------------------------------------------------------------------*/
        // Connection success and start to communication.
        printf("\n=> Welcome!! Enter # to end the connection \n\n");

        msg_queue msg_wts;      // queue for the massages are wating to send.
        msg_wts.clear();

        bool end_connection = false;

        // Start a new thread for sending message
        scoped_thread sendThread(thread(send_TCP, ref(msg_wts), ref(client_helper), ref(client_fd), ref(end_connection)));

        //Timout clocker
        scoped_thread timeoutThread(thread(client_helper.timeout_clocker, ref(end_connection)));

        // Listern incomming message
        while(!end_connection)
        {
            int is_msg_usable = client_helper.recv_msg(client_fd);
            if (is_msg_usable == -1)
            {
                // Socket error or loss conection.
                end_connection = true;
            }
            else if (is_msg_usable == 1)
            {
                if (client_helper.msg_incomplete.substr(0,3).compare("RSP")){
                    cout << "Message "<< static_cast<int>(client_helper.ID_msg_incomplete) <<" from server :" << client_helper.msg_incomplete << endl;

                    // Responde to server when get msg.
                    string RSP = "RSP";
                    string respond = RSP + client_helper.ID_msg_incomplete;
                    msg_wts.push_respond(respond);
                }
                else
                {
                    // Delete key message timeout.
                    client_helper.msg_confirm(client_helper.msg_incomplete);
                };
            };
        };
        is_finsh = is_reconnect(client_fd);
    };

    /*------------------------------------------------------------------------------------------------------------*/
    /*----- Close the server socket and exit the program -----*/
    freeaddrinfo(server_infor);
    printf("\n=> Socket is closed.\n=> Goodbye...\n");
    return 0;
}
