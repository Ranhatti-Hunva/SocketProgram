TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    md5.cpp \
    handlemsg.cpp \
    clientchat.cpp

HEADERS += \
    md5.h \
    handlemsg.h \
    clientchat.h \
    threadpool.h
