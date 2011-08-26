/**
 * @file NDLCom/include/NDLCom/communication_statistic_widget.h
 * @author Armin Burchardt
 * @date 2011
 */
#ifndef _NDLCOM__COMMUNICATION_STATISTIC_WIDGET_H_
#define _NDLCOM__COMMUNICATION_STATISTIC_WIDGET_H_

#include "NDLCom/message.h"

#include <QWidget>
#include <QMap>
#include <QList>
#include <QTime>
#include <QTimer>

namespace NDLCom
{
    namespace Ui
    {
        class CommunicationStatisticWidget;
    };

    class CommunicationStatisticWidget : public QWidget
    {
        Q_OBJECT

            public:
        CommunicationStatisticWidget(QWidget *parent=NULL);

    public slots:
        void rxMessage(const ::NDLCom::Message&);

    private slots:
        /**
         * Empty table.
         */
        void on_resetButton_clicked();

        /**
         * Update of rate information even if no data was received.
         */
        void on_mpTimerRateUpdate_timeout();

    private:
        Ui::CommunicationStatisticWidget *mpUi;

        /**
         * Timer to trigger updateRateColumn().
         */
        QTimer* mpTimerRateUpdate;

        void createTableHeader();

        QMap<long, int> lastFrameCounter;
        QMap<long, int> counterMissed;
        QMap<long, int> counterReceived;
        QMap<long, double> estimatedFrequency;
        //QMap<long, double> estimatedJitter;


        /**
         * Map from keys (senderid,receiverid,messageid) to line in gui table.
         */
        QMap<long, int> keyToLine;

        /**
         * Map from key to estimated frequency.
         */
        QMap<long, QList<double> > lastFrequencies;

        /**
         * Map from key to a list of timestamps.
         */
        QMap<long, QList<struct timespec> > lastTimeStamps;

        /**
         * Map frmo key to timestamp.
         */
        QMap<long, struct timespec> lastTimeReceived;
    };
};

#endif/*_NDLCOM__COMMUNICATION_STATISTIC_WIDGET_H_*/

