TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    clientchat.cpp \
    handlemsg.cpp \
    md5.cpp

HEADERS += \
    clientchat.h \
    handlemsg.h \
    md5.h \
    threadpool.h
