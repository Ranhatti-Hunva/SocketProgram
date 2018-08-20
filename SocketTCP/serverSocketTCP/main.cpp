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
#include "usercommand.h"
#include "tcphelper.h"

using namespace std;
std::mutex user_command_muxtex;
std::mutex fd_set_muxtex;
std::condition_variable cond;

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
    std::vector <int> client_fds;

    fd_set master;
    fd_set read_fds;

    const int stdin = 0;

    FD_ZERO(&master);
    FD_ZERO(&read_fds);
    client_fds.clear();

    FD_SET(server_fd, &master);
    FD_SET(stdin,&master);

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    int fdmax = (server_fd > stdin) ? server_fd : stdin;

    // Set non-blocking
    long arg = fcntl(server_fd, F_GETFL, NULL);
    arg |= O_NONBLOCK;
    fcntl(server_fd, F_SETFL, arg);

    user_command user_command;
    user_command.clear();

    scoped_thread sendThread(std::thread(send_TCP,std::ref(user_command), std::ref(client_socket_list), std::ref(master), std::ref(fdmax), std::ref(client_fds), std::ref(server_helper)));

    while(!user_command.compare("#"))
    {
        std::unique_lock<mutex> locker_fd_set(fd_set_muxtex,std::defer_lock);
        locker_fd_set.lock();
        read_fds = master;
        if (select(fdmax+1,&read_fds, nullptr, nullptr, &tv) <0)
        {
            perror("=> Select");
            exit(EXIT_FAILURE);
        }
        locker_fd_set.unlock();

        if (FD_ISSET(stdin, &read_fds))
        {
            string inputStr;
            getline(cin, inputStr);

            std::unique_lock<mutex> locker(user_command_muxtex);
            user_command.set(inputStr);
            locker.unlock();
            cond.notify_all();
        };

        if (FD_ISSET(server_fd, &read_fds))
        {
            server_helper.acceptor(server_fd, client_fds, master, fdmax, client_socket_list);
        };

        for (unsigned long i = 0; i< client_fds.size(); i++)
        {
            if(FD_ISSET(client_fds[i], &read_fds))
            {
                const unsigned int bufsize = 256;
                char buffer[bufsize];
                memset(&buffer,0,bufsize/sizeof(char));
                long status;

                if ((status=recv(client_fds[i], buffer, bufsize,0)) <=0)
                {
                    if ((status == -1) && ((errno == EAGAIN)|| (errno == EWOULDBLOCK)))
                    {
                        perror("=> Message is not gotten complete !!");
                    }
                    else
                    {
                        if (status == 0)
                        {
                            printf("=> Connect client has been close, soket: %d\n",client_fds[i]);
                        }
                        else
                        {
                            printf("=> Error socket.");
                        };

                        locker_fd_set.lock();

                        FD_CLR(client_fds[i], &master);

                        client_socket_list.delete_fs_num(client_fds[i]);
                        close(client_fds[i]);
                        client_fds.erase(client_fds.begin()+static_cast<long>(i));

                        locker_fd_set.unlock();
                    };
                }
                else
                {
                    std::string msg_incompleted;
                    int is_unpacked = server_helper.unpacked_msg(buffer, msg_incompleted);
                    if (is_unpacked) {
                        cout << "Message from client, socket " << client_fds[i] << ":" << msg_incompleted << endl;
                        process_on_buffer_recv(msg_incompleted.c_str(),client_socket_list, client_fds[i], user_command);
                    };

//                    cout << "Message from client, socket " << client_fds[i] << ":" << buffer << endl;
//                    process_on_buffer_recv(buffer,client_socket_list, client_fds[i], user_command);
                };
            };
        };
    };
    /*------------------------------------------------------------------------------------------------------------*/
    close(server_fd);
    printf("=> Closed the server socket!! \n");
}


