#include "RegRepPlugin.h"

#include <QtQml>
#include <QQmlEngine>
#include <QQmlContext>

#include "constantsregistry.h"
#include "report.h"
#include "reportsreceiver.h"

void RegRepPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(QLatin1String(uri) == QLatin1String("RegRep"));

    qmlRegisterType<ReportsReceiver>(uri, 1, 0, "ReportsReceiver");

    qRegisterMetaType<ConstantEntry>("ConstantEntry");
    qRegisterMetaType<Report>("Report");

    // Lowercase name: Report is a Q_GADGET/value type, and Qt6's QML type
    // system expects value types to use a lowercase name (like "point", "rect").
    qmlRegisterUncreatableType<Report>(uri, 1, 0, "report",
        QStringLiteral("Report is a value type — read it from ReportsReceiver.onReportReceived"));
}

void RegRepPlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    Q_UNUSED(uri)
    engine->rootContext()->setContextProperty(QStringLiteral("ConstantRegistry"),
                                              &ConstantRegistry::instance());
    engine->rootContext()->setContextProperty(QStringLiteral("Constants"),
                                              ConstantRegistry::instance().map());
}
