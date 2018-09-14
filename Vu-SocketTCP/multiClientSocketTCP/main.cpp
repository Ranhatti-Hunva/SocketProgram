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
string user_name[NUM_CLIENT];
bool is_error[NUM_CLIENT];
bool end_connection[NUM_CLIENT];
bool reconnect[NUM_CLIENT];
int ping_pong[NUM_CLIENT];

int main()
{
    // Notice the program information.
    printf("\n                    Multi_client TCP       \n");
    printf("\n              -------------**--------------\n\n");

    thread_pool threads_master(NUM_CLIENT+1);

    for(int i=0; i< NUM_CLIENT; i++)
    {
        user_name[i] = "D" + std::to_string(i);
        end_connection[i] = false;
        is_error[i] = false;
        reconnect[i] = true;
        ping_pong[i] = 0;
    }

    // Read terminal.
    threads_master.enqueue(read_terminal);

    bool is_stop_process = false;

    while(!is_stop_process)
    {
        for (int client_oder = 0; client_oder < NUM_CLIENT; client_oder++)
        {
            if(true == reconnect[client_oder])
            {
                threads_master.enqueue([=]()
                {
                    TCPclient client_helper; // Help client for any TCP process needed.

                    /*------------------------------------------------------------------------------------------------------------*/
                    struct addrinfo *server_infor, *p;
                     server_infor = client_helper.get_addinfo_list("10.42.0.187",1500);
//                     server_infor = client_helper.get_addinfo_list("10.42.0.127",8096);
//                    server_infor = client_helper.get_addinfo_list("",1500);
                    p = server_infor;

                    /*------------------------------------------------------------------------------------------------------------*/
                    end_connection[client_oder] = false;
                    is_error[client_oder] = false;
                    ping_pong[client_oder] = 0;

                    // Thread pool
                    thread_pool threads(5);

                    while (!end_connection[client_oder])
                    {
                        // Queue for the massages are wating to send.
                        msg_queue msg_wts;
                        msg_wts.clear(Q_MSG);
                        msg_wts.clear(Q_RSP);

                        int client_fd;

                        // read terminal
                        threads.enqueue(load_cmd, client_oder, ref(client_helper), ref(msg_wts), ref(threads));

                        // Connection with timeout to server.
                        while(!end_connection[client_oder])
                        {
                            client_fd = client_helper.connect_with_timeout(p);
                            if(client_fd < 0)
                            {
                                if(end_connection[client_oder])
                                {
                                    break;
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
                                printf("=> Client %s have connected to server. \n", user_name[client_oder].c_str());
                                break;
                            }
                        };
                        if(end_connection[client_oder]) break;                        
                        /*------------------------------------------------------------------------------------------------------------*/
                        // Connection success and start to communication.
                        threads.enqueue([&]()
                        {
                            client_helper.send_msg(msg_wts, client_oder, client_fd);
                        });

                        threads.enqueue([&]()
                        {
                            client_helper.timeout_clocker(client_oder);
                        });

                        threads.enqueue([&]()
                        {
                            client_helper.get_msg_buffer(client_oder, msg_wts, threads);
                        });

                        // Listern incomming message
                        while(!end_connection[client_oder])
                        {
//                            client_helper.send_msg(msg_wts, client_oder, client_fd);

                            int is_disconnect = client_helper.recv_msg(client_fd, msg_wts, threads, client_oder);
                            if(is_disconnect < 0)
                            {
                                end_connection[client_oder] = true;
                                is_error[client_oder] = true;
                                sleep(2);
                                break;
                            }
                            usleep(1);
                        };

                        if (is_error[client_oder])
                        {
                            // Restart the process because the error
                            end_connection[client_oder] = false;
                            is_error[client_oder] = false;
                            close(client_fd);
                        }
                        else
                        {
                            close(client_fd);
                        };
                    };

                    /*------------------------------------------------------------------------------------------------------------*/
                    freeaddrinfo(server_infor);
                });
                reconnect[client_oder] = false;
                std::this_thread::sleep_for(std::chrono::milliseconds(20));
            }
            else
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            };
        };

        is_stop_process = true;
        for(int i=0; i<NUM_CLIENT; i++)
        {
            if(false == end_connection[i] )
            {
                is_stop_process = false;
                break;
            };
        };
    };

    printf("=> Socket is closed.\n=> Goodbye...\n");
    return 0;
}
