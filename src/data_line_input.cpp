/**
 * @file widgets/src/data_line_input.cpp
 * @author Tobias Stark
 * @date 2010
 */
#include "data_line_input.h"
#include <stdio.h>
#include "hex_input.h"
#include <QWidget>

NDLCom::DataLineInput::DataLineInput(QWidget *parent) : HexInput(parent)
{
    setDisplayStyle(HEX);
}

void NDLCom::DataLineInput::setDataLength(int length)
{
    while (length > data().length()) {
        char buf[2];
        sprintf(buf,"%02x",data().length()+1);
        insert(buf);
    }

    if (length < data().length()) {
        setText(text().left(length*3));
    }
}

