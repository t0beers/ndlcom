/**
 * @file NDLCom/include/NDLCom/hex_input.h
 * @class NDLCom::HexInput
 *
 * @brief A QLineEdit to insert text in ascii or hex.
 *
 * This class is a QLineEdit that has an internal QByteArray
 * buffer which contains the data to display.
 * With the @link HexInput::setDisplayStyle() setDisplayStyle @endlink
 * function the display style can be switched between @link HexInput::ASCII
 * ASCII @endlink and @link HexInput::HEX HEX @endlink .\n
 *
 * NOTE: With displayStyle == ASCII 0x00 where replaced by 0xff to display
 * the data. The buffer remains unchanged. (FIXME)\n
 *
 * German Research Center for Artificial Intelligence\n
 * Project: iStruct\n
 *
 * $LastChangedRevision: 1348 $
 * $LastChangedBy: tstark $
 * $LastChangedDate: 2011-04-14 19:57:23 +0200 (Thu, 14 Apr 2011) $
 *
 * @author Tobias Stark (tobias.stark@dfki.de)
 * @date 01.01.2011
 *
 */
#ifndef _NDLCOM_HEXINPUT_H_
#define _NDLCOM_HEXINPUT_H_

#include <QLineEdit>
#include <QList>

namespace NDLCom
{

    class HexInput : public QLineEdit
    {
        Q_OBJECT

            public:
        HexInput(QWidget *parent=NULL);

        enum viewType { ASCII, HEX };
        void setDisplayStyle(viewType type);

        /**
         * @brief Get the buffered data.
         *
         * Get the data out of the objects internal buffer.
         * @return the buffered data
         */
        QByteArray data() { return buffer; }
        void setData(const QByteArray &data);

        public slots:
        void clear();

    signals:
        void dataLengthChanged(int length);

    protected:
        virtual bool event(QEvent *e);

        protected slots:
        void inputChanged(const QString &text);

    private:
        char hexStringToAscii(const char *hex);
        void asciiToHexString(const char ascii, char *hex);

        viewType displayStyle;
        QByteArray buffer;

        QList<QByteArray> historyBuffer;
        int historyIndex;

    };
};

#endif/*_NDLCOM_HEXINPUT_H_*/

/**
 * @}
 * @}
 */

