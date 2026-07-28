#pragma once
#include <QObject>
#include <QList>

#include "report.h"
#include "regrep_global.h"

class ReportsReceiver;

// Hidden report bus. Not exposed to QML directly — Reporter (and anything
// else) publishes reports into it, and ReportsReceiver instances register
// themselves to receive the ones matching their filters.
class REGREP_EXPORT ReportRouter : public QObject
{
    Q_OBJECT

public:
    static ReportRouter &instance()
    {
        static ReportRouter s_instance;
        return s_instance;
    }

    void publish(const Report &r);

    // Called by ReportsReceiver's constructor/destructor.
    void registerReceiver(ReportsReceiver *receiver);
    void unregisterReceiver(ReportsReceiver *receiver);

private:
    explicit ReportRouter(QObject *parent = nullptr) : QObject(parent) {}

    QList<ReportsReceiver *> m_receivers;
};
