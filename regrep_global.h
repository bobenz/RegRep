#pragma once
#include <QtCore/qglobal.h>

#ifndef REGREP_EXPORT
#  if defined(REGREP_LIBRARY)
#    define REGREP_EXPORT Q_DECL_EXPORT   // building the DLL
#  elif defined(REGREP_DLL)
#    define REGREP_EXPORT Q_DECL_IMPORT   // consuming the DLL
#  else
#    define REGREP_EXPORT                 // source-inclusion / static build
#  endif
#endif
