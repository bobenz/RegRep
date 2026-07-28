#include "reportsreceiver.h"
#include "reportrouter.h"

ReportsReceiver::ReportsReceiver(QObject *parent) : QObject(parent)
{
    ReportRouter::instance().registerReceiver(this);
}

ReportsReceiver::~ReportsReceiver()
{
    ReportRouter::instance().unregisterReceiver(this);
}

QString ReportsReceiver::sourceFilter() const { return m_sourceFilter; }

void ReportsReceiver::setSourceFilter(const QString &pattern)
{
    if (m_sourceFilter == pattern) return;
    m_sourceFilter = pattern;
    m_sourceRegex.setPattern(pattern);
    m_sourceRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    emit sourceFilterChanged();
}

QString ReportsReceiver::categoryFilter() const { return m_categoryFilter; }

void ReportsReceiver::setCategoryFilter(const QString &pattern)
{
    if (m_categoryFilter == pattern) return;
    m_categoryFilter = pattern;
    m_categoryRegex.setPattern(pattern);
    m_categoryRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    emit categoryFilterChanged();
}

QString ReportsReceiver::messageFilter() const { return m_messageFilter; }

void ReportsReceiver::setMessageFilter(const QString &pattern)
{
    if (m_messageFilter == pattern) return;
    m_messageFilter = pattern;
    m_messageRegex.setPattern(pattern);
    m_messageRegex.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
    emit messageFilterChanged();
}

bool ReportsReceiver::matchesPattern(const QRegularExpression &regex, const QString &value)
{
    if (regex.pattern().isEmpty())
        return true;
    return regex.match(value).hasMatch();
}

bool ReportsReceiver::matches(const Report &r) const
{
    return matchesPattern(m_sourceRegex, r.source)
        && matchesPattern(m_categoryRegex, r.category)
        && matchesPattern(m_messageRegex, r.message);
}

void ReportsReceiver::deliver(const Report &r)
{
    emit reportReceived(r);
}
