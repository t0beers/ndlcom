/**
 * @file NDLCom/src/communication_statistic_widget.cpp
 * @brief 
 * @author Armin Burchardt
 * @date 2011
 */
#include "NDLCom/communication_statistic_widget.h"

#include "protocol/protocol.h"
#include "representations/names.h"

#include <stdio.h>
#include <cmath>
#include <time.h>

#include "ui_communication_statistic_widget.h"

#include <QDebug>

NDLCom::CommunicationStatisticWidget::CommunicationStatisticWidget(QWidget *parent) : QWidget(parent)
{
    mpTimerRateUpdate = new QTimer(this);
    mpTimerRateUpdate->setObjectName("mpTimerRateUpdate");

    mpUi = new NDLCom::Ui::CommunicationStatisticWidget();
    mpUi->setupUi(this);

    mpUi->outputTable->setColumnCount(7);
    createTableHeader();

    mpTimerRateUpdate->start(100); //unit: milliseconds, e.g. 10Hz
}

void NDLCom::CommunicationStatisticWidget::createTableHeader()
{
    QTableWidgetItem *headerItem;
    headerItem = new QTableWidgetItem("Sender");
    mpUi->outputTable->setHorizontalHeaderItem(0,headerItem);
    headerItem = new QTableWidgetItem("Receiver");
    mpUi->outputTable->setHorizontalHeaderItem(1,headerItem);
    headerItem = new QTableWidgetItem("Message ID");
    mpUi->outputTable->setHorizontalHeaderItem(2,headerItem);
    headerItem = new QTableWidgetItem("Received");
    mpUi->outputTable->setHorizontalHeaderItem(3,headerItem);
    headerItem = new QTableWidgetItem("Missed");
    mpUi->outputTable->setHorizontalHeaderItem(4,headerItem);
    headerItem = new QTableWidgetItem("Error Ratio");
    mpUi->outputTable->setHorizontalHeaderItem(5,headerItem);
    headerItem = new QTableWidgetItem("   Rate   ");
    mpUi->outputTable->setHorizontalHeaderItem(6,headerItem);
    // This makes no sense yet, because we have huge deviations due to missing real time functionality
    //headerItem = new QTableWidgetItem("StdDev");
    //mpUi->outputTable->setHorizontalHeaderItem(7,headerItem);
    mpUi->outputTable->resizeColumnsToContents();
}

void NDLCom::CommunicationStatisticWidget::on_resetButton_clicked()
{
    lastFrameCounter.clear();
    keyToLine.clear();
    lastTimeReceived.clear();
    counterReceived.clear();
    counterMissed.clear();
    estimatedFrequency.clear();
    //estimatedJitter.clear();    
    mpUi->outputTable->clear();
    mpUi->outputTable->setRowCount(0);
    createTableHeader();
}

void NDLCom::CommunicationStatisticWidget::rxMessage(const Message& msg)
{
    /* carefull: moved timestamping out of this function, but didn't revise the code... */
    struct timespec timestamp = msg.mTimestamp;
    ProtocolHeader header = msg.mHdr;
    char* data = msg.mpDecodedData;

    uint8_t recvMessageId = header.mDataLen > 0 ? data[0] : 0;
    const long mapKey = header.mSenderId<<16 | header.mReceiverId<<8 | recvMessageId;
    bool firstPacket = !counterReceived.contains(mapKey);

    if(firstPacket)
    {
        counterReceived[mapKey] = 1;
        counterMissed[mapKey] = 0;
        lastTimeReceived[mapKey] = timestamp;
        lastFrameCounter[mapKey] = header.mCounter;
        //lastFrequencies[mapKey] = QList<double>();
        lastTimeStamps[mapKey] = QList<struct timespec>();
    }
    else
    {
        counterReceived[mapKey]++;
        int lastCounter = lastFrameCounter[mapKey];
        int diffCounter = (header.mCounter - lastCounter);
        int missed = 0;
        if (diffCounter < 0)
        {
            //handle overflow
            diffCounter += 256;
        }
        if (diffCounter > 1)
        {
            printf("missed package: last: %d act: %d\n",lastCounter,header.mCounter);
            missed = diffCounter - 1;
        }
        counterMissed[mapKey] += missed;

        /*
         * add timestamp to list of timestamps and limit list
         * length (larger lists have less noise in rate estimation
         * but reaction and calculation is slower).
         */        
        lastTimeStamps[mapKey].append(timestamp);
        if (lastTimeStamps[mapKey].size() > 250)
        {
            lastTimeStamps[mapKey].removeFirst();
        }

        /*
         * Get timestamp of oldest packet in list and calculate a
         * mean rate from it.
         * Good for high rates.
         */
        const struct timespec& tv_oldest = lastTimeStamps[mapKey].first();
        double since_oldest = (timestamp.tv_nsec - tv_oldest.tv_nsec) * 1.e-9
            + (timestamp.tv_sec - tv_oldest.tv_sec);
        double rate_from_oldest = lastTimeStamps[mapKey].size() / since_oldest;
        /*
         * Calculate rate based on timestamp of last packet.
         * Good for low rates.
         */
        struct timespec tv_last = lastTimeReceived[mapKey];
        long unsigned delta_nanos = timestamp.tv_nsec - tv_last.tv_nsec;
        long unsigned delta_millis = (timestamp.tv_sec - tv_last.tv_sec) * 1000;
        double rate_from_last = 1000. /(delta_millis + delta_nanos/1.e6);

        double rateMean = rate_from_last > 30 ? rate_from_oldest : rate_from_last;
        //double rateVariance = 0.;
        //double rateStdDev = 0.;

        /* update list of frequencies and estimate variance. */
        //QList<double>& list = lastFrequencies[mapKey];
        //list.append(rate_from_last);
        //for (int i = 0; i < list.size(); ++i)
        //{
            //double entryDiff = (list[i] - rateMean);
            //rateVariance += entryDiff * entryDiff;
        //}
        //rateVariance /= list.size() - 1;
        //rateStdDev = sqrt(rateVariance);
        //if(list.size() > 250)
        //{
            //list.removeFirst();
        //}

        estimatedFrequency[mapKey] = rateMean; //estimatedFrequency[mapKey] * 0.99 + 0.01 * rate;
        //estimatedJitter[mapKey] = rateStdDev;
        lastFrameCounter[mapKey] = header.mCounter;
        lastTimeReceived[mapKey] = timestamp;
    }
}

void NDLCom::CommunicationStatisticWidget::on_mpTimerRateUpdate_timeout()
{
    struct timespec tv_now;
    int r = clock_gettime(CLOCK_REALTIME, &tv_now);
    Q_ASSERT(r == 0);

    //create lines for new data-flows.
    const QList<long>& keys = counterReceived.keys();
    for (int i = 0; i < keys.size(); i++)
    {
        const long key = keys[i];
        if (!keyToLine.contains(key))
        {
            int line = mpUi->outputTable->rowCount();
            mpUi->outputTable->setRowCount(line + 1);
            uint8_t senderId = (key >> 16) & 0xff;
            uint8_t receiverId = (key >> 8) & 0xff;
            uint8_t reprId = key & 0xff;
            const char* senderName = representationsNamesGetDeviceName(senderId);
            const char* receiverName = representationsNamesGetDeviceName(receiverId);
            const char* representationName = representationsNamesGetRepresentationName(reprId);
            QTableWidgetItem* newSenderItem = new QTableWidgetItem(senderName ? senderName : QString::number(senderId));
            QTableWidgetItem* newReceiverItem = new QTableWidgetItem(receiverName ? receiverName : QString::number(receiverId));
            QTableWidgetItem* newRepresentationItem = new QTableWidgetItem(representationName ? representationName : QString::number(reprId));
            QTableWidgetItem *newReceivedItem = new QTableWidgetItem("1");
            QTableWidgetItem *newMissedItem = new QTableWidgetItem("0");
            QTableWidgetItem *newErrorRatioItem = new QTableWidgetItem("0%");
            QTableWidgetItem *newRateItem = new QTableWidgetItem("0 Hz");
            newReceivedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            newMissedItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            newErrorRatioItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            newRateItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            mpUi->outputTable->setItem(line,0,newSenderItem);
            mpUi->outputTable->setItem(line,1,newReceiverItem);
            mpUi->outputTable->setItem(line,2,newRepresentationItem);
            mpUi->outputTable->setItem(line,3,newReceivedItem);
            mpUi->outputTable->setItem(line,4,newMissedItem);
            mpUi->outputTable->setItem(line,5,newErrorRatioItem);
            mpUi->outputTable->setItem(line,6,newRateItem);
            //mpUi->outputTable->setItem(line,7,new QTableWidgetItem(""));

            mpUi->outputTable->resizeColumnsToContents();
            keyToLine.insert(key, line);
        }
    }

    //update every line
    for (int i = 0; i < keys.size(); i++)
    {
        const long key = keys[i];
        int line = keyToLine[key];
        //update receive-counter
        int received = counterReceived[key];
        mpUi->outputTable->item(line,3)->setText(QString::number(received));
        //update missed-counter
        int missed = counterMissed[key];
        mpUi->outputTable->item(line,4)->setText(QString::number(missed));
        //display percentage of missed packets
        int expected = received + missed;
        float missedPercent = 0.f;
        if (expected > 0)
        {
            missedPercent = 100.f * missed / expected;
        }
        mpUi->outputTable->item(line,5)->setText(QString::number(missedPercent,'f',2) + "%");        
        
        struct timespec tv_last = lastTimeReceived[key];
        long delta_nanos = tv_now.tv_nsec - tv_last.tv_nsec;
        long delta_millis = (tv_now.tv_sec - tv_last.tv_sec) * 1000;
        const double rate = 1000. /(delta_millis + delta_nanos/1.e6);

        //update if the last packet was received before the last
        //updateRateColumn()-call was triggered. Otherwise the column will
        //be updated at the arrival of packets.
        if (rate < 1000./mpTimerRateUpdate->interval())
        {
            mpUi->outputTable->item(line,6)->setText(QString("%1 Hz").arg(rate, 7, ' ', 1));
            //mpUi->outputTable->item(line,7)->setText(QString(""));
        }
        else
        {
            double rate = estimatedFrequency[key];
            //double jitter = estimatedJitter[key];
            mpUi->outputTable->item(line,6)->setText(QString("%1 Hz").arg(rate, 7, ' ', 1));
            //mpUi->outputTable->item(line,7)->setText(QString("%1 Hz").arg(jitter, 7, ' ', 1));
        }
    }
}

