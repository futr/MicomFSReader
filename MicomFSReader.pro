#-------------------------------------------------
#
# Project created by QtCreator 2014-08-01T12:06:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MicomFSReader
TEMPLATE = app


SOURCES += main.cpp\
        widget.cpp \
    micomfs_dev.c \
    micomfs.c \
    progressdialog.cpp \
    logicaldrivedialog.cpp

HEADERS  += widget.h \
    micomfs_dev.h \
    micomfs.h \
    progressdialog.h \
    logicaldrivedialog.h

FORMS    += widget.ui \
    progressdialog.ui \
    logicaldrivedialog.ui
