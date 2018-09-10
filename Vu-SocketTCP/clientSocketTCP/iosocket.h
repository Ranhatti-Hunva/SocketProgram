#ifndef IOSOCKET_H
#define IOSOCKET_H

#include <iostream>
#include <string.h>
#include <vector>
#include <fcntl.h>
#include <dirent.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/event.h>
#include <unistd.h>

#include "msgqueue.h"
#include "tcphelper.h"
#include "threadpool.h"

extern std::string user_name;

struct file_infor
{
    std::string name;
    struct stat attr;
};

static vector<file_infor> files;

void splits_string(const std::string& subject, std::vector<std::string>& container);

void read_terminal(bool& end_connection, TCPclient& client_helper, msg_queue& msg_wts, thread_pool& threads);

bool is_reconnect(int& client_fd);

void send_from_file(bool& stop_read_file, const std::string user_forward, TCPclient& client_helper, msg_queue& msg_wts);

std::vector<file_infor> getDirectoryFiles(const std::string& dir, const char* ext);

bool is_wts(vector<file_infor> files_list, file_infor file);

bool push_file(file_infor file, TCPclient& client_helper, msg_queue& msg_wts, string user_forward);

#endif // IOSOCKET_H
