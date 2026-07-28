#include "reportrouter.h"
#include "reportsreceiver.h"

void ReportRouter::publish(const Report &r)
{
    const auto receivers = m_receivers;
    for (ReportsReceiver *receiver : receivers) {
        if (receiver->matches(r))
            receiver->deliver(r);
    }
}

void ReportRouter::registerReceiver(ReportsReceiver *receiver)
{
    if (!m_receivers.contains(receiver))
        m_receivers.append(receiver);
}

void ReportRouter::unregisterReceiver(ReportsReceiver *receiver)
{
    m_receivers.removeAll(receiver);
}
