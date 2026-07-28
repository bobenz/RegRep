#ifndef REGREP_REGISTRATION_H
#define REGREP_REGISTRATION_H
#pragma once
#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>

#include "constantsregistry.h"
#include "report.h"
#include "reportsreceiver.h"

static const char* regrep_uri   = "RegRep";
static const int   regrep_major = 1;
static const int   regrep_minor = 0;

// Registers the RegRep types/context-properties directly, for projects that
// source-include RegRep instead of loading it as a QML plugin (mirrors
// ConcertoRegistration in QmlConcerto).
class RegRepRegistration
{
public:
    RegRepRegistration(QQmlEngine* engine)
    {
        Q_ASSERT(engine);
        qmlRegisterType<ReportsReceiver>(regrep_uri, regrep_major, regrep_minor, "ReportsReceiver");

        qRegisterMetaType<ConstantEntry>("ConstantEntry");
        qRegisterMetaType<Report>("Report");

        // Expose the registry itself so QML can call declare()/lookup()/etc.
        engine->rootContext()->setContextProperty("ConstantRegistry", &ConstantRegistry::instance());

        // Expose the PropertyMap as "Constants" for dot-notation access, e.g.
        // Constants.shutter_stuck.description
        engine->rootContext()->setContextProperty("Constants", ConstantRegistry::instance().map());

#ifdef REGREP_HOME
        engine->addImportPath(REGREP_HOME);
#endif
    }
};

#endif // REGREP_REGISTRATION_H
