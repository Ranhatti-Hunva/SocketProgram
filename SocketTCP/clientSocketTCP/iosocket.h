#ifndef IOSOCKET_H
#define IOSOCKET_H

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
extern std::string user_name;

void send_TCP(user_command& user_command, fd_set& master, int& socket_fd);

int is_reconnect(int& client_fd);

bool unpacked_msg(char* buffer, std::string& msg_incomplete);

#endif // IOSOCKET_H
