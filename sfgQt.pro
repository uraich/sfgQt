#-------------------------------------------------
#
# Project created by QtCreator 2019-07-07T11:08:12
#
#-------------------------------------------------

QT += core gui

QMAKE_CFLAGS += -std=c99
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += serialport

TARGET = sfgQt
TEMPLATE = app
RESOURCES = images.qrc
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
INCLUDEPATH += driver

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += c++11

SOURCES += \
    main.cpp \
    sfgQt.cpp \
    driver/fpSerial.cpp \
    driver/fpProtocol.cpp \
    enrolldialog.cpp \
    notepaddialog.cpp \
    verifyTemplatedialog.cpp \
    writenotepaddialog.cpp \
    driver/printmsg.cpp

HEADERS += \
    sfgQt.h \
    driver/fpProtocol.h \
    driver/fpSerial.h \
    enrolldialog.h \
    notepaddialog.h \
    verifyTemplateDialog.h \
    writenotepaddialog.h \
    driver/printmsg.h

FORMS += \
    enrollDialog.ui \
    sfgQt.ui \
    notepaddialog.ui \
    verifyTemplateDialog.ui \
    writenotepaddialog.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
