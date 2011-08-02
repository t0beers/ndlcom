/**
 * @file lib/NDLCom/include/NDLCom/composer.h
 * @brief Telegram- or MessageComposer
 * @author Martin Zenzes
 * @date 2011-07-28
 */
#ifndef _NDLCOM_COMPOSER_H_
#define _NDLCOM_COMPOSER_H_

#include "NDLCom/message.h"

#include <QAction>
#include <QTimer>

namespace NDLCom
{
    /** handcrufted QLineEdit to allow hex-inputs */
    class DataLineInput;

    namespace Ui
    {
        class Composer;
    };

    /**
     * @brief MessageComposer to create arbritrary messages and send them
     *
     * allows usage of any combination of values for a NDLCom::Message
     *
     * allows mass-sending of the same Message (optionally with increasing PaketCounter) with
     * adjustable sending rate.
     *
     * we have two QComboBox which automagically display all known NDLCom-devices, to be selected as
     * receivers.
     *
     * every composed packet will be emitted as a NDLCom::Message.
     */
    class Composer : public QWidget
    {
        Q_OBJECT
    public:
        Composer(QWidget* parent = 0);
        virtual ~Composer() {};

    signals:
        void txMessage(const ::NDLCom::Message&);

    private:
        Ui::Composer* mpUi;

        QAction* actionSend;
        QTimer* mpSendTimer;
        DataLineInput* mpDataInput;

        /* and here is the private, internal mechanic */
    private slots:
        void on_actionSend_triggered();
        void on_autoSend_toggled(bool);
        void on_sendRate_valueChanged(int);
        void on_receivers_currentIndexChanged(int);
        void on_senders_currentIndexChanged(int);
        void on_senderId_valueChanged(int);
        void on_receiverId_valueChanged(int);
    };
};

#endif/*_NDLCOM_COMPOSER_H_*/

