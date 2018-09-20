TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    clientchat.cpp \
    handlemsg.cpp \
    md5.cpp \
    msgqueue.cpp
QMAKE_MAC_SDK = macosx10.13.6

HEADERS += \
    clientchat.h \
    handlemsg.h \
    md5.h \
    threadpool.h \
    msgqueue.h
