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
#include <QThread>
#include <QMutex>

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 * @addtogroup Communication_NDLCom_Composer composer
 * @{
 */

#define NSEC_PER_SEC    (1000000000) /* The number of nsecs per sec. */

namespace NDLCom
{
    /** handcrufted QLineEdit to allow hex-inputs */
    class DataLineInput;
    class SendThread;

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
        ~Composer();

    signals:
        void txMessage(const NDLCom::Message&);

    private:
        Ui::Composer* mpUi;

        QAction* actionSend;
        SendThread* mpSendThread;
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

    /**
     * SendThread -- a little hacked class to provide a possibility for a high resolution timer
     *
     * similar to QTimer, but we have "setFrequency" to set. We don't know signle shots.
     *
     * Inherits QTread, thus is more heavyweight than normal QTimer.
     */
    class SendThread : public QThread
    {
    Q_OBJECT

    public:
        SendThread(QObject *parent=0, unsigned int frequency=1);
        ~SendThread();
        /**
         * set this to true to halt the thread in a controlled manner
         */
        bool stopRunning;
        /**
         *  setFrequency -- changing the frequency in which timeout() is emitted.
         *
         * @param frequency frequency in Hz
         */
        void setFrequency(unsigned int frequency);

    signals:
        /**
         *  timeout -- called with the set frequency, if the thread is running
         */
        void timeout();
    private:
        void run();
        unsigned int mFrequency;
        QMutex freqMutex;
    };
};

/**
 * @}
 * @}
 * @}
 */
#endif/*_NDLCOM_COMPOSER_H_*/

