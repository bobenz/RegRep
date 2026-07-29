QT      += quick qml
CONFIG  += c++17
INCLUDEPATH += $$PWD
DEFINES     += "REGREP_HOME=\\\"$$PWD\\\"" REGREP_DLL

# Link against the import lib in the source-relative lib/ folder.
# Build RegRep first so lib/debug or lib/release is populated.
win32:CONFIG(debug, debug|release) {
    LIBS += -L$$PWD/lib/debug   -lRegRep
    REGREP_DLL_SRC  = $$PWD/lib/debug/RegRep.dll
    REGREP_DLL_DEST = $$OUT_PWD/debug
} else {
    LIBS += -L$$PWD/lib/release -lRegRep
    REGREP_DLL_SRC  = $$PWD/lib/release/RegRep.dll
    REGREP_DLL_DEST = $$OUT_PWD/release
}
# Explicit DESTDIR (e.g. QmlConcerto.pro) wins over the default per-config subdir.
!isEmpty(DESTDIR): REGREP_DLL_DEST = $$DESTDIR

# Copy RegRep.dll next to whatever links against it, so it's found at
# runtime — Windows searches the executable's own directory first.
regrep_dll_copy.files = $$REGREP_DLL_SRC
regrep_dll_copy.path  = $$REGREP_DLL_DEST
COPIES += regrep_dll_copy
