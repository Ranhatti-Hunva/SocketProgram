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

#include "msgqueue.h"
#include "tcphelper.h"

extern std::string user_name;

void send_TCP(msg_queue& msg_wts, TCPclient& client_helper, int& socket_fd, bool& finish);

int is_reconnect(int& client_fd);

#endif // IOSOCKET_H
