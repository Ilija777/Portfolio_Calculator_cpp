QT += core gui widgets serialport

CONFIG += c++17 release console
CONFIG -= app_bundle
QMAKE_CXXFLAGS_RELEASE += -g  # Debugging-Informationen für Release hinzufügen
TARGET = Calculator_Application
# Im Projektordner
INCLUDEPATH += $$PWD/src
SOURCES += main.cpp \
           src/mainwindow.cpp

HEADERS += src/mainwindow.h


