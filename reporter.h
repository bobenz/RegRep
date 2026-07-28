#pragma once
#include <QString>
#include <QVariant>

#include "report.h"
#include "constantsregistry.h"
#include "regrep_global.h"

// Instance-based report emitter. Owns a "source" tag (e.g. a path like
// "Sequence/Withdrawal/Dispense") and builds+publishes Report values through
// ReportRouter — the composable alternative to baking reporting directly into
// a base class. Cheap value type; embed one as a member wherever reports need
// to be emitted:
//
//   class Phrase : public QObject {
//       ...
//       Reporter m_reporter{"Sequence/Withdrawal"};
//       void someMethod() { m_reporter.info("started"); }
//   };
class REGREP_EXPORT Reporter
{
public:
    explicit Reporter(const QString &source = QString()) : m_source(source) {}

    QString source() const { return m_source; }
    void setSource(const QString &source) { m_source = source; }

    void info(const QString &msg, const QVariant &data = QVariant()) const
    {
        log(Report::Info, msg, data);
    }

    void warning(const QString &msg, const QVariant &data = QVariant()) const
    {
        log(Report::Warning, msg, data);
    }

    void error(const ConstantEntry &entry) const
    {
        log(Report::Error, entry.description(), QVariant::fromValue(entry));
    }

    void debug(const QString &msg, const QVariant &data = QVariant()) const
    {
        log(Report::Debug, msg, data);
    }

    void critical(const QString &msg, const QVariant &data = QVariant()) const
    {
        log(Report::Critical, msg, data);
    }

    void log(Report::Category category, const QString &msg, const QVariant &data = QVariant()) const;

private:
    QString m_source;
};
