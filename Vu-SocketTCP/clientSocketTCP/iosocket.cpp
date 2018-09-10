#include "iosocket.h"

// using to analyser user command.
void splits_string(const std::string& subject, std::vector<std::string>& container)
{
    container.clear();
    size_t len = subject.length()+ 1;
    char* s = new char[ len ];
    memset(s, 0, len*sizeof(char));
    memcpy(s, subject.c_str(), (len - 1)*sizeof(char));
    for (char *p = strtok(s, "/"); p != nullptr; p = strtok(nullptr, "/"))
    {
        container.push_back(p);
    }
    delete[] s;
}

void read_terminal(bool& end_connection, TCPclient& client_helper, msg_queue& msg_wts, thread_pool& threads)
{
    bool stop_read_file;
    while(!end_connection)
    {
        fd_set reader;
        FD_ZERO(&reader);
        FD_SET(0,&reader);

        struct timeval general_tv;

        general_tv.tv_sec = 0;
        general_tv.tv_usec = 5000;

        int data = select(1,&reader, nullptr, nullptr, &general_tv);
        if (data > 0)
        {
            if (FD_ISSET(0, &reader))
            {
                std::string user_cmd_str;
                getline(std::cin, user_cmd_str);

                if(!user_cmd_str.empty())
                {                    
                    if(!user_cmd_str.compare("#"))
                    {
                        end_connection = true;
                        is_error = false;
                        stop_read_file = true;
                        break;
                    }
                    else
                    {                                              
                        // Packet msg and push to msg_send_queue
                        std::vector<std::string> container;
                        splits_string(user_cmd_str, container);

                        if(container.size() < 2)
                        {
                            printf("=> Wrong format message or no massge to send !! \n");
                        }
                        else
                        {
                            if (!container[0].compare("file"))
                            {
                                stop_read_file = false;
                                threads.enqueue(send_from_file, ref(stop_read_file), container[1], ref(client_helper), ref(msg_wts));
                            }
                            else if (!container[0].compare("un_file"))
                            {
                                stop_read_file = true;
                            }
                            else
                            {
                                msg_text msg_send;
                                msg_send.msg = user_cmd_str;
                                msg_send.type_msg = MSG;

                                std::vector<unsigned char> element;
                                client_helper.packed_msg(msg_send, element);
                                msg_wts.push(element, Q_MSG);
                            };
                        };
                    };
                };
            };
        };
    };
};

bool is_reconnect(int& client_fd)
{
    // Ask user if they want to reconnect with server after lose connection.
    std::string answer;
    while (1)
    {
        close(client_fd);
        printf("=> Do you want reconnect to server (Y/N): ");
        getline(std::cin, answer);

        if(!answer.compare("Y"))
        {
            return false;
        }
        else if (!answer.compare("N"))
        {
            return true;
        }
        else
        {
            printf("\n=> Don't understand the answer.\n");
            continue;
        }
    };
};

std::vector<file_infor> getDirectoryFiles(const std::string& dir, const char* ext) {
  std::vector<file_infor> files;
  std::shared_ptr<DIR> directory_ptr(opendir(dir.c_str()), [](DIR* dir){ dir && closedir(dir); });
  struct dirent *dirent_ptr;
  if (!directory_ptr) {
    std::cout << "Error opening : " << std::strerror(errno) << dir << std::endl;
    return files;
  }

  char *p;

  while ((dirent_ptr = readdir(directory_ptr.get())) != nullptr) {

      size_t len = std::string(dirent_ptr->d_name).length()+ 1;
      char* s = new char[ len ];
      memset(s, 0, len*sizeof(char));
      memcpy(s, std::string(dirent_ptr->d_name).c_str(), (len - 1)*sizeof(char));

      p=strtok(dirent_ptr->d_name,".");

      if (p != nullptr)
      {
          p = strtok(nullptr,".");
          if(p != nullptr)
          {
              if(!strcmp(p,ext))
              {
                 file_infor file;
                 file.name = dir + "/" + std::string(dirent_ptr->d_name)+".txt";

                 std::string path = file.name;
                 stat(path.c_str(),&file.attr);

                 files.push_back(file);
              };
          };
      };

  }
  return files;
};

bool is_wts(vector<file_infor> files_list, file_infor file)
{
    for(unsigned long i = 0; i<files_list.size(); i++)
    {
        if(files_list[i].name ==  file.name)
        {
            if(file.attr.st_mtime != files_list[i].attr.st_mtime)
            {
                return true;
            }
            else
            {
                return false;
            };
        };
    };
    return true;
};

bool push_file(file_infor file, TCPclient& client_helper, msg_queue& msg_wts, string user_forward)
{
    int fd;
    off_t bytes_readed;
    const int bufsize = 1024;
    char buffer[bufsize] = {0};

    if ((fd = open(file.name.c_str(),O_RDONLY))==-1)
    {
        perror("=>open fail");
        return false;
    };
    bytes_readed = read(fd,buffer,bufsize);

    while(bytes_readed)
    {
        msg_text msg_send;
        msg_send.msg = user_forward + "/" + string(buffer);
        msg_send.type_msg = MSG;

        std::vector<unsigned char> element;
        client_helper.packed_msg(msg_send, element);
        msg_wts.push(element, Q_MSG);

        memset(&buffer, 0, sizeof(buffer));

        bytes_readed = read(fd,buffer,bufsize);
    };
    close(fd);
    return true;
};

void send_from_file(bool& stop_read_file, const std::string user_forward, TCPclient& client_helper, msg_queue& msg_wts)
{
    // Send all file for the first time
    vector<file_infor> files = getDirectoryFiles("../build-clientSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/filemsg", "txt");
    for (unsigned long i=0; i< files.size(); i++)
    {
        push_file(files[i], client_helper, msg_wts, user_forward);
    };

    // Check change in foder and start resend file
    const char *dirname = "../build-clientSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/filemsg/";
    int kq = kqueue ();
    int dirfd = open (dirname, O_RDONLY);

    struct kevent direvent;
    EV_SET (&direvent, dirfd, EVFILT_VNODE, EV_ADD | EV_CLEAR | EV_ENABLE,NOTE_WRITE, 0, (void *)dirname);

    kevent(kq, &direvent, 1, nullptr, 0, nullptr);

    struct timespec time_delay;
    time_delay.tv_sec = 1;
    time_delay.tv_nsec = 0;

    while(!stop_read_file)
    {
        struct kevent change;
        memset(&change, 0, sizeof(change));

        if (kevent(kq, nullptr, 0, &change, 1, &time_delay) == -1)
        {
            // Erorr on kevent
            stop_read_file = true;
            break;
        }
        else if (change.udata != nullptr)
        {
            // There is something change in file foder.
            vector<file_infor> new_files;

            new_files = getDirectoryFiles("../build-clientSocketTCP-Desktop_Qt_5_11_1_clang_64bit-Debug/filemsg", "txt");

            for (unsigned long i=0; i< new_files.size(); i++)
            {
                if(is_wts(files, new_files[i]))
                {
                    push_file(new_files[i], client_helper, msg_wts, user_forward);
                };
            };
            files = new_files;
        };
    };
}









