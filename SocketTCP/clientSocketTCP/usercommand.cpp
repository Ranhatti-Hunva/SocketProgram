#include "usercommand.h"

void user_command::set(std::string str){
    std::lock_guard<std::mutex> guard(userInputMutex);
    userCommand = str;
}

void user_command::clear(){
    std::lock_guard<std::mutex> guard(userInputMutex);
    userCommand.clear();
}

bool user_command:: empty(){
    std::lock_guard<std::mutex> guard(userInputMutex);
    return userCommand.empty();
}

std::string user_command::get(){
    std::lock_guard<std::mutex> guard(userInputMutex);
    return userCommand;
}

bool user_command::compare(std::string str){
    std::lock_guard<std::mutex> guard(userInputMutex);
    return !userCommand.compare(str);
}
