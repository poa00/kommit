#include "gitglobal.h"
#include <QCalendar>
#include <QColor>
#include <QMap>

#pragma once

class KommitWidgetsGlobalOptions
{
public:
    KommitWidgetsGlobalOptions();

    void setColor(Git::ChangeStatus status, const QColor &color);
    Q_REQUIRED_RESULT QColor statucColor(Git::ChangeStatus status) const;

    Q_REQUIRED_RESULT QCalendar calendar() const;
    void setCalendar(const QCalendar &calendar);

    static KommitWidgetsGlobalOptions *instance();

private:
    QMap<Git::ChangeStatus, QColor> mColors;
    QCalendar mCalendar;
};
