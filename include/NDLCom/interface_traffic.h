/**
 * @file include/NDLCom/interface_traffic.h
 * @author zenzes
 * @date 2011
 */

#ifndef _NDLCOM_INTERFACE_TRAFFIC_H_
#define _NDLCOM_INTERFACE_TRAFFIC_H_

#include <QWidget>
#include <QSyntaxHighlighter>

namespace NDLCom
{
    class Message;
    class Highlighter;

    namespace Ui
    {
        class Traffic;
    };

    class InterfaceTraffic : public QWidget
    {

        Q_OBJECT
    public:
        InterfaceTraffic(QWidget* parent = 0);
        virtual ~InterfaceTraffic() {};

    public slots:
        void txTraffic(const QByteArray&);
        void rxTraffic(const QByteArray&);

    private:
        Ui::Traffic* mpUi;

        Highlighter* mpHighlight_Rx;
        Highlighter* mpHighlight_Tx;

    };


    class Highlighter : public QSyntaxHighlighter
    {
        Q_OBJECT

        public:
            Highlighter(QTextDocument *parent = 0);

        protected:
            void highlightBlock(const QString &text);

        private:
            struct HighlightingRule
            {
                QRegExp pattern;
                QTextCharFormat format;
            };
            QVector<HighlightingRule> highlightingRules;
    };
};

#endif/*_NDLCOM_INTERFACE_TRAFFIC_H_*/
