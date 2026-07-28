#pragma once
#include <QString>
#include <QVariant>
#include <QDateTime>
#include <QDebug>

#include "regrep_global.h"

// A single reportable event: who emitted it (source), what kind (category),
// a human-readable message, an optional free-form payload (data), and when.
// Decoupled from any particular emitter — anything can build one and hand it
// to ReportRouter::publish(), whether via a Reporter instance or directly.
struct REGREP_EXPORT Report {
    Q_GADGET
    Q_PROPERTY(QString source MEMBER source)
    Q_PROPERTY(QString category MEMBER category)
    Q_PROPERTY(QString message MEMBER message)
    Q_PROPERTY(QVariant data MEMBER data)
    Q_PROPERTY(QDateTime timestamp MEMBER timestamp)
public:
    enum Category {
        Info,
        Warning,
        Error,
        Debug,
        Critical
    };
    Q_ENUM(Category)
    QString source;
    QString category;
    QString message;
    QVariant data;
    QDateTime timestamp;
};

Q_DECLARE_METATYPE(Report)

inline QDebug operator<<(QDebug debug, const Report &report)
{
    QDebugStateSaver saver(debug);
    debug.nospace() << "Report("
                    << "Source: "    << report.source
                    << ", Category: " << report.category
                    << ", Message: "  << report.message
                    << ", Data: "     << report.data
                    << ", Time: "     << report.timestamp.toString(Qt::ISODateWithMs)
                    << ")";
    return debug;
}
