QT      += quick qml
CONFIG  += c++17
INCLUDEPATH += $$PWD
DEFINES     += "REGREP_HOME=\\\"$$PWD\\\"" REGREP_DLL

# Link against the import lib in the source-relative lib/ folder.
# Build RegRep first so lib/debug or lib/release is populated.
win32:CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/lib/debug   -lRegRep
} else {
    LIBS += -L$$PWD/lib/release -lRegRep
}
