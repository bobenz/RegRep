QT += testlib qml quick
CONFIG -= app_bundle
CONFIG += qmltestcase
TEMPLATE = app

include(../regrep.pri)

DEFINES += SRCDIR=\\\"$$PWD\\\"

SOURCES += main.cpp

DISTFILES += \
    tst/tst_ConstantsRegistry.qml \
    tst/tst_ReportsReceiver.qml
