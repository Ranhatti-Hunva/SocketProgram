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

// Time function, sockets, htons... file stat
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/stat.h>

// File function and bzero
#include <strings.h>
#include <future>

#include "msgqueue.h"
#include "tcphelper.h"

extern std::string user_name;

void splits_string(const std::string& subject, std::vector<std::string>& container);

void read_terminal(bool& end_connection, TCPclient& client_helper, msg_queue& msg_wts);

bool is_reconnect(int& client_fd);

#endif // IOSOCKET_H
