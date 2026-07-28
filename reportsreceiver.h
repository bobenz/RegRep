#pragma once
#include <QObject>
#include <QString>
#include <QRegularExpression>

#include "report.h"
#include "regrep_global.h"

// QML-instantiable element that listens for reports on the (hidden)
// ReportRouter, filtered by regex against a Report's fields. All filters are
// optional (empty pattern = match-all) and combined with AND.
//
// Example:
//   ReportsReceiver {
//       sourceFilter: "CDM/.*"
//       categoryFilter: "Error|Critical"
//       onReportReceived: (report) => console.log(report.message)
//   }
class REGREP_EXPORT ReportsReceiver : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString sourceFilter   READ sourceFilter   WRITE setSourceFilter   NOTIFY sourceFilterChanged)
    Q_PROPERTY(QString categoryFilter READ categoryFilter WRITE setCategoryFilter NOTIFY categoryFilterChanged)
    Q_PROPERTY(QString messageFilter  READ messageFilter  WRITE setMessageFilter  NOTIFY messageFilterChanged)

public:
    explicit ReportsReceiver(QObject *parent = nullptr);
    ~ReportsReceiver() override;

    QString sourceFilter() const;
    void setSourceFilter(const QString &pattern);

    QString categoryFilter() const;
    void setCategoryFilter(const QString &pattern);

    QString messageFilter() const;
    void setMessageFilter(const QString &pattern);

    // Called by ReportRouter — not meant for direct QML/C++ use.
    bool matches(const Report &r) const;
    void deliver(const Report &r);

signals:
    void reportReceived(Report report);

    void sourceFilterChanged();
    void categoryFilterChanged();
    void messageFilterChanged();

private:
    static bool matchesPattern(const QRegularExpression &regex, const QString &value);

    QString m_sourceFilter;
    QString m_categoryFilter;
    QString m_messageFilter;

    QRegularExpression m_sourceRegex;
    QRegularExpression m_categoryRegex;
    QRegularExpression m_messageRegex;
};
