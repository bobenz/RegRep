#include "reporter.h"
#include "reportrouter.h"

#include <QDateTime>
#include <QMetaEnum>

void Reporter::log(Report::Category category, const QString &msg, const QVariant &data) const
{
    Report r;
    r.source    = m_source;
    r.category  = QString::fromLatin1(QMetaEnum::fromType<Report::Category>().valueToKey(category));
    r.message   = msg;
    r.data      = data;
    r.timestamp = QDateTime::currentDateTime();
    ReportRouter::instance().publish(r);
}
