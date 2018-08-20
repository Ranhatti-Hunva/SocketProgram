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

#include "clientmanager.h"
#include "iosocket.h"
#include "msgqueue.h"
#include "tcphelper.h"

using namespace std;

class  scoped_thread
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
    // Create server echo socket on port 1500
    TCPserver server_helper;
    int server_fd = server_helper.server_echo(1500);

    /*------------------------------------------------------------------------------------------------------------*/
    // Connect and communiaction with client.
    client_list client_socket_list;

    // Create a message queue to send for socket.
    msg_queue msg_wts;
    msg_wts.clear();

    bool finish = false;

    scoped_thread sendThread(std::thread(send_TCP,ref(msg_wts),ref(client_socket_list),ref(server_helper), ref(finish)));

    while(!finish)
    {
        server_helper.reciver(server_fd, client_socket_list, msg_wts);
    };
    /*------------------------------------------------------------------------------------------------------------*/
    printf("=> Closed the server socket!! \n");
}


