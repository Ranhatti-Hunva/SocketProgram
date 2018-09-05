#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string>
#include <arpa/inet.h>
#include <sys/types.h>
#include <errno.h>
#include <arpa/inet.h>
#include <thread>
#include <mutex>
#include <vector>
#include <fcntl.h>

#include "threadpool.h"
#include "clientmanager.h"
#include "iosocket.h"
#include "msgqueue.h"
#include "tcphelper.h"

using namespace std;

int main()
{
    printf("\n              -------------**--------------               \n\n");
    // Create server echo socket on port 1500.
    TCPserver server_helper;
    int server_fd = server_helper.server_echo(1500);

    /*------------------------------------------------------------------------------------------------------------*/
    // Connect and communiaction with client.
    client_list client_socket_list;

    // Create a message queue to send for socket.
    msg_queue msg_wts;
    msg_wts.clear(Q_MSG);
    msg_wts.clear(Q_RSP);

    // Flage stop server
    bool end_connection = false;

    // Thread pool
    thread_pool threads(10);

    threads.enqueue(read_terminal, ref(end_connection), ref(client_socket_list), ref(server_helper), ref(msg_wts));

    threads.enqueue([&]()
    {
        server_helper.send_msg(msg_wts, end_connection, client_socket_list, threads);
    });

    threads.enqueue([&]()
    {
        server_helper.timeout_clocker(end_connection, client_socket_list);
    });

    while(!end_connection)
    {
        server_helper.recv_msg(server_fd, client_socket_list, msg_wts, threads);
    };

    /*------------------------------------------------------------------------------------------------------------*/
    close(server_fd);
    printf("=> Closed the server socket!! \n");
}


