/**
 * @file lib/NDLCom/src/hex_input.cpp
 * @author Tobias Stark
 * @date 2010
 */
#include "hex_input.h"
#include <QKeyEvent>
#include <stdio.h>

/**
 * @brief HexInput ctor
 */
NDLCom::HexInput::HexInput(QWidget *parent) : QLineEdit(parent)
{
    displayStyle = ASCII;
    historyIndex = 0;

    connect(this,SIGNAL(textChanged(const QString&)),SLOT(inputChanged(const QString &)));
}

/**
 * @brief Function to select if data should be displayed in ASCII
 *        or HEX mode.
 *
 * @param type new type of the display (ASCII or HEX)
 */
void NDLCom::HexInput::setDisplayStyle(viewType type)
{
    //if (type == displayStyle) return;
  
    displayStyle = type;

    switch (displayStyle) {

    case ASCII:
        QLineEdit::setText(buffer);
        break;

    case HEX:
        char hex[2];
        QString hexText;
        for (int i=0; i<buffer.length(); i++) {
            asciiToHexString(buffer[i],hex);
            hexText.append(hex);
            hexText.append(" ");
        }
        QLineEdit::setText(hexText);

        break;
    }
}

void NDLCom::HexInput::setData(const QByteArray &data)
 {
     buffer = data;

     // redraw
     setDisplayStyle(displayStyle);
}

/**
 * @brief Removes all contents from the input and the buffer.
 */
void NDLCom::HexInput::clear()
{
    buffer.clear();
    QLineEdit::clear();
}

bool NDLCom::HexInput::event(QEvent *e)
{
    if (e->type() == QEvent::KeyPress) {

        QKeyEvent *key = (QKeyEvent *)e;

        if ((key->key() == Qt::Key_Return) || (key->key() == Qt::Key_Enter)) {
            historyIndex = 0;
            historyBuffer.push_front(buffer);
            
            return QLineEdit::event(e);
        } else if (key->key() == Qt::Key_Up) {
            historyIndex++;
            if (historyIndex>historyBuffer.length()-1)
                historyIndex = historyBuffer.length()-1;
            buffer = historyBuffer[historyIndex];
            setDisplayStyle(displayStyle);

        } else if (key->key() == Qt::Key_Down) {
            historyIndex--;
            if (historyIndex<0)
                historyIndex = 0;
            buffer = historyBuffer[historyIndex];
            setDisplayStyle(displayStyle);

        } else if (displayStyle == HEX) {

            QChar c = key->text()[0];

            if ((c.category() == QChar::Number_DecimalDigit) ||
                (c.category() == QChar::Letter_Uppercase) ||
                (c.category() == QChar::Letter_Lowercase)) {

                if ((key->key() == Qt::Key_0) || (key->key() == Qt::Key_1) ||
                    (key->key() == Qt::Key_2) || (key->key() == Qt::Key_3) ||
                    (key->key() == Qt::Key_4) || (key->key() == Qt::Key_5) ||
                    (key->key() == Qt::Key_6) || (key->key() == Qt::Key_7) ||
                    (key->key() == Qt::Key_8) || (key->key() == Qt::Key_9) ||
                    (key->key() == Qt::Key_A) || (key->key() == Qt::Key_B) ||
                    (key->key() == Qt::Key_C) || (key->key() == Qt::Key_D) ||
                    (key->key() == Qt::Key_E) || (key->key() == Qt::Key_F)) {

                    // only insert key - spaces will be inserted by inputChanged slot
                    QLineEdit::insert(key->text());
                }

            } else if (c.category() == QChar::Other_Control)
                return QLineEdit::event(e);

            return true;
        }
    }

    // normal key event handling
    return QLineEdit::event(e);
}

char NDLCom::HexInput::hexStringToAscii(const char *hex)
{
    int hexint[2];

    for (int i=0; i<2; i++) {
        switch (hex[i]) {
        case '0': hexint[i] = 0;  break;
        case '1': hexint[i] = 1;  break;
        case '2': hexint[i] = 2;  break;
        case '3': hexint[i] = 3;  break;
        case '4': hexint[i] = 4;  break;
        case '5': hexint[i] = 5;  break;
        case '6': hexint[i] = 6;  break;
        case '7': hexint[i] = 7;  break;
        case '8': hexint[i] = 8;  break;
        case '9': hexint[i] = 9;  break;
        case 'a':
        case 'A':
            hexint[i] = 10; break;
        case 'b':
        case 'B':
            hexint[i] = 11; break;
        case 'c':
        case 'C':
            hexint[i] = 12; break;
        case 'd':
        case 'D':
            hexint[i] = 13; break;
        case 'e':
        case 'E':
            hexint[i] = 14; break;
        case 'f':
        case 'F':
            hexint[i] = 15; break;
        default:
            printf("ERROR(hexToAscii): char '%c' not hex\n",hex[i]);
        }
    }

    return hexint[0]*16+hexint[1];
}

void NDLCom::HexInput::asciiToHexString(const char ascii, char *hex)
{
    sprintf(hex,"%02x",(unsigned char)ascii);
}

void NDLCom::HexInput::inputChanged(const QString &text)
{
    switch (displayStyle) {

    case ASCII:
        buffer = text.toAscii();
        break;

    case HEX:
        QString buf = text.toAscii();
        bool changed=false;
        int oldCursorPosition = cursorPosition();

        for (int i=0; i<buf.length(); i++) {
            if (i%3<2) {
                // should not be a space
                if (buf[i]==' ') {
                    buf.remove(i,1);
                    if (i<oldCursorPosition) oldCursorPosition++;
                    i--;
                    changed=true;
                }
            } else {
                // should be a space
                if (buf[i]!=' ') {
                    buf.insert(i,' ');
                    if (i<oldCursorPosition) oldCursorPosition++;
                    i--;
                    changed=true;
                }
            }
        }

        if (changed) {
            // update text input
            setText(buf);
            setCursorPosition(oldCursorPosition);
        } else {
            // update buffer
            buffer.clear();
            for (int i=0; i<buf.length()-1; i+=3)
                buffer.append(hexStringToAscii(buf.mid(i,2).toAscii().constData()));
            emit dataLengthChanged(buffer.size());
        }
        break;
    }
}

