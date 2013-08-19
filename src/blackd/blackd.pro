QT       += core gui xml svg network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets


TARGET = blackd
TEMPLATE = app
CONFIG += c++11

DEPENDPATH += . ..
INCLUDEPATH += ..

SOURCES  += *.cpp
HEADERS  += *.h

FORMS    += blackd.ui
RESOURCES += blackd.qrc

LIBS += -L../../lib -lblackcore -lblackmisc

win32:!win32-g++*: PRE_TARGETDEPS += ../../lib/blackmisc.lib \
                                     ../../lib/blackcore.lib
else:              PRE_TARGETDEPS += ../../lib/libblackmisc.a \
                                     ../../lib/libblackcore.a

DESTDIR = ../../bin



