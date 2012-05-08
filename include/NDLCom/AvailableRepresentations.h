#ifndef AVAILABLE_REPRESENTATIONS_H
#define AVAILABLE_REPRESENTATIONS_H

#include <QWidget>

namespace Representations
{
};

namespace NDLCom
{
    namespace Ui
    {
        class AvailableRepresentations;
    };

    /* just a simple Widget, which prints all devices and representation types... with their id */
    class AvailableRepresentations : public QWidget
    {
        Q_OBJECT
    public:
        AvailableRepresentations(QWidget* parent=NULL);
    };
};

#endif /*AVAILABLE_REPRESENTATIONS_H*/
