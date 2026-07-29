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

# Deploy: RegRep.dll + RegRep.lib -> $$DEPLOY_LIB_DIR (shared with every other
# plugin's DLL — regrep_dll.pri links consumers against the .lib found there),
# qmldir -> $$DEPLOY_ROOT/RegRep/ (its "plugin RegRep ../lib" line points back at it).
# The DLL/lib copy MUST be QMAKE_POST_LINK, not COPIES — COPIES treats its .files as
# static pre-existing sources, and pointing it at this project's own just-built
# DLL creates a dependency cycle ("cycle in dependency tree for target ...dll").
# (COPIES is fine for qmldir below — that's a static source file, not a build output.)
include(deploy.pri)

QMAKE_POST_LINK = cmd /c \
    "(if not exist $$shell_path($$DEPLOY_LIB_DIR) mkdir $$shell_path($$DEPLOY_LIB_DIR)) \
    && copy /y $$shell_path($$DESTDIR/RegRep.dll) $$shell_path($$DEPLOY_LIB_DIR) \
    && copy /y $$shell_path($$DESTDIR/RegRep.lib) $$shell_path($$DEPLOY_LIB_DIR)\\"

regrep_qmldir_deploy.files = $$PWD/qmldir
regrep_qmldir_deploy.path  = $$DEPLOY_ROOT/RegRep
COPIES += regrep_qmldir_deploy
