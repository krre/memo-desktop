QT += core gui widgets

TARGET = memo
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++17

SOURCES += \
        main.cpp \
    ui/MainWindow.cpp

HEADERS += \
    core/Constants.h \
    ui/MainWindow.h
