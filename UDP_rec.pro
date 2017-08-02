#-------------------------------------------------
#
# Project created by QtCreator 2017-04-05T15:38:38
#
#-------------------------------------------------

QT       += core gui
QT += network
QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = UDP_rec
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp \
    readini.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h \
    readini.h

FORMS    += mainwindow.ui
