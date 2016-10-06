#-------------------------------------------------
#
# Project created by QtCreator 2016-08-20T15:08:43
#
#-------------------------------------------------

QT       += core gui
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = bindiff-qt

TEMPLATE = app

*-g++*:QMAKE_CXXFLAGS += -Wall -Weffc++ -Wextra -Wconversion -Wsign-conversion -std=c++14

SOURCES += main.cpp\
    mainwindow.cpp \
    binfileview.cpp

HEADERS  += mainwindow.h \
    binfileview.h

FORMS    += mainwindow.ui

OTHER_FILES += license.txt

INSTALLS += target

unix{
    target.path = /bin
    message( The executable will be installed to $$target.path )
}
