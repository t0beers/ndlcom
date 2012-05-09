#include "NDLCom/StaticTools.h"

#include <QIcon>
#include <QString>
#include <QPainter>

/* another sexy hack: showing the number of active interfaces in the actionConnect*-actions */
QIcon NDLCom::printNumberOnIcon(const QIcon &icon, int number, QColor color)
{
    /* we don't paint zeros... */
    if (number == 0)
        return icon;

    /* this may be unefficient, but it works... */
    QPixmap pix = icon.pixmap(32,32);
    QPainter painter;
    painter.begin(&pix);
    painter.setPen(color);
    painter.setFont(QFont("", 14, -1, false));
    painter.drawText(QPoint(0,20), QString::number(number));
    painter.end();
    return QIcon(pix);
}

/* little hack for niceness! */
QString NDLCom::sizeToString(int size)
{
    int bytes = 1;
    int kbytes = 1024*bytes;
    int mbytes = 1024*kbytes;
    int gbytes = 1024*mbytes;

    if (size > gbytes)
        return QString::number((double)(size)/gbytes,'f',2)+QString(" GiB");
    else if (size > mbytes)
        return QString::number((double)(size)/mbytes,'f',2)+QString(" MiB");
    else if (size > kbytes)
        return QString::number((double)(size)/kbytes,'f',2)+QString(" KiB");
    else
        return QString::number(size)+QString(" B");
}
