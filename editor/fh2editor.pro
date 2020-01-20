######################################################################
# Automatically generated by qmake (2.01a) ?? ????. 9 09:58:39 2012
######################################################################

TEMPLATE = app
TARGET = fh2editor

DEPENDPATH += .
INCLUDEPATH += .

QT += widgets
QT += xml

CONFIG += qt

QMAKE_LIBDIR += $(QTLIB)
QMAKE_CXXFLAGS += -fno-strict-aliasing

RESOURCES = resources.qrc
FORMS +=

win32-g++-cross {
    CONFIG += console
}

linux-g++ {
}

QMAKE_CXX = g++
QMAKE_LINK = g++

LIBS = -L/usr/local/Cellar/qt/5.13.2/lib -framework QtWidgets -framework QtGui -framework QtXml -framework QtCore -framework DiskArbitration -framework IOKit -framework OpenGL -framework AGL

# Input
HEADERS += mainwindow.h mapwindow.h mapdata.h dialogs.h
SOURCES += program.cpp engine.cpp mainwindow.cpp mapwindow.cpp mapdata.cpp dialogs.cpp global.cpp
