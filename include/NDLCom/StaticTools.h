#ifndef STATICTOOLS_H
#define STATICTOOLS_H

#include <QColor>

class QString;
class QColor;
class QIcon;

namespace NDLCom
{
    extern QIcon printNumberOnIcon(const QIcon &icon, int number, QColor color = Qt::black);
    extern QString sizeToString(int size);
};


#endif /*STATICTOOLS_H*/
