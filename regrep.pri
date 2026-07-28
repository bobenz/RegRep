# Ensure C++17 and required Qt modules are present in the parent project
QT += quick qml
CONFIG += c++17

# Define the include path so the parent project can find headers easily
INCLUDEPATH += $$PWD

# Header files
HEADERS += \
    $$PWD/regrep_global.h \
    $$PWD/constantsregistry.h \
    $$PWD/report.h \
    $$PWD/reportrouter.h \
    $$PWD/reportsreceiver.h \
    $$PWD/reporter.h \
    $$PWD/regrep_registration.h

# Source files
SOURCES += \
    $$PWD/reportrouter.cpp \
    $$PWD/reportsreceiver.cpp \
    $$PWD/reporter.cpp

# QML module descriptor — listed so Qt Creator shows it in the project tree
DISTFILES += \
    $$PWD/qmldir

DEFINES += "REGREP_HOME=\\\"$$PWD\\\""
