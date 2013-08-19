QT      += core gui xml svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

INCLUDEPATH += . ..

TARGET = blackbox
TEMPLATE = app
CONFIG += c++11

SOURCES += *.cpp
HEADERS += *.h

FORMS   += blackbox.ui dialog_connect.ui dialog_chat.ui

LIBS    += -L../../lib -lblackmisc

win32:!win32-g++*: PRE_TARGETDEPS += ../../lib/blackmisc.lib
else:              PRE_TARGETDEPS += ../../lib/libblackmisc.a

DESTDIR = ../../bin
