TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    serverchat.cpp \
    clientmanage.cpp \
    handlemsg.cpp \
    msgqueue.cpp

HEADERS += \
    serverchat.h \
    clientmanage.h \
    handlemsg.h \
    clientnode.h \
    threadpool.h \
    msgqueue.h
