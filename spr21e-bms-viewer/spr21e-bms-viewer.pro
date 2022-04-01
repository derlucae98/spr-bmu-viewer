QT       += core gui serialbus network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

MAJOR=0
MINOR=0
BUILD=$$system(date +%F_%H%M%S)
AUTHOR="Luca Engelmann"
#BUILD=$$system(date -R)

DEFINES += "VERS_MAJOR=\"$$MAJOR\"" "VERS_MINOR=\"$$MINOR\"" "VERS_BUILD=\"\\\"$$BUILD\\\"\"" "AUTHORS=\"\\\"$$AUTHOR\\\"\""

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    can.cpp \
    diagdialog.cpp \
    logfileconverter.cpp \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    aboutdialog.h \
    can.h \
    diagdialog.h \
    logfileconverter.h \
    mainwindow.h

FORMS += \
    aboutdialog.ui \
    diagdialog.ui \
    logfileconverter.ui \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    rsc.qrc
