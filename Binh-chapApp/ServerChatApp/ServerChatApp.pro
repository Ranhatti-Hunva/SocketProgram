TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        main.cpp \
    clientmanage.cpp \
    handlemsg.cpp \
    msgqueue.cpp \
    serverchat.cpp
QMAKE_MAC_SDK = macosx10.13.6

HEADERS += \
    clientmanage.h \
    clientnode.h \
    handlemsg.h \
    msgqueue.h \
    serverchat.h \
    threadpool.h
