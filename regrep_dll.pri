QT      += quick qml
CONFIG  += c++17
INCLUDEPATH += $$PWD
DEFINES     += "REGREP_HOME=\\\"$$PWD\\\"" REGREP_DLL

# Deploy root — see deploy.pri for CNGO_DIR/DEPLOY_ROOT/DEPLOY_LIB_DIR.
include($$PWD/deploy.pri)

# Link against the import lib in the shared deploy tree — RegRep.pro's own
# post-link step puts RegRep.lib/.dll there, so build RegRep first.
LIBS += -L$$DEPLOY_LIB_DIR -lRegRep
REGREP_DLL_SRC = $$DEPLOY_LIB_DIR/RegRep.dll
win32:CONFIG(debug, debug|release): REGREP_DLL_DEST = $$OUT_PWD/debug
else:                                REGREP_DLL_DEST = $$OUT_PWD/release
# Explicit DESTDIR (e.g. QmlConcerto.pro) wins over the default per-config subdir.
!isEmpty(DESTDIR): REGREP_DLL_DEST = $$DESTDIR

# Copy RegRep.dll next to whatever links against it, so it's found at
# runtime — Windows searches the executable's own directory first.
regrep_dll_copy.files = $$REGREP_DLL_SRC
regrep_dll_copy.path  = $$REGREP_DLL_DEST
COPIES += regrep_dll_copy
