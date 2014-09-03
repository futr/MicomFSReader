#-------------------------------------------------
#
# Project created by QtCreator 2014-08-01T12:06:51
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MicomFSReader
TEMPLATE = app

TRANSLATIONS = mfs_ja.ts

SOURCES += main.cpp\
        widget.cpp \
    micomfs_dev.c \
    micomfs.c \
    progressdialog.cpp \
    logicaldrivedialog.cpp \
    writefileworker.cpp

HEADERS  += widget.h \
    micomfs_dev.h \
    micomfs.h \
    progressdialog.h \
    logicaldrivedialog.h \
    writefileworker.h

FORMS    += widget.ui \
    progressdialog.ui \
    logicaldrivedialog.ui

OTHER_FILES += \
    win.rc \
    MicomFSReader.exe.manifest

win32 {
    RC_FILE = win.rc
}

RESOURCES += \
    resource.qrc


