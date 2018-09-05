#ifndef THREADFUNCTION_H
#define THREADFUNCTION_H

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
#include <fstream>
#include <stack>

#include "clientmanager.h"
#include "msgqueue.h"
#include "tcphelper.h"

extern std::mutex user_command_muxtex;
extern std::mutex fd_set_muxtex;
extern std::condition_variable cond;

void splits_string(const std::string& subject, std::vector<std::string>& container);

void read_terminal(bool& end_connection, client_list& client_socket_list, TCPserver& server_helper, msg_queue& msg_wts);

#endif // THREADFUNCTION_H
