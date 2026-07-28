TARGET   = RegRep
TEMPLATE = lib
CONFIG  += plugin c++17

# Import lib (.lib) — link-time only, stays in the source tree
win32:CONFIG(debug, debug|release): DESTDIR = $$PWD/lib/debug
else:                                DESTDIR = $$PWD/lib/release

# Export macro so all classes get Q_DECL_EXPORT when building the DLL
DEFINES += REGREP_LIBRARY

# Shared module sources, headers, and resources
include(regrep.pri)

# Plugin entry point
HEADERS += RegRepPlugin.h
SOURCES += RegRepPlugin.cpp

# Copy qmldir alongside the built plugin binary so `import RegRep 1.0` can
# find it during local testing (no external deploy target wired up yet).
qmldir_copy.files = $$PWD/qmldir
win32:CONFIG(debug, debug|release): qmldir_copy.path = $$PWD/lib/debug
else:                                qmldir_copy.path = $$PWD/lib/release
COPIES += qmldir_copy
