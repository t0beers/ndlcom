/**
 * @file src/data_line_input.cpp
 * @author Tobias Stark
 * @date 2010
 */
#ifndef _NDLCOM__DATALINEINPUT_H_
#define _NDLCOM__DATALINEINPUT_H_

#include "hex_input.h"

namespace NDLCom
{
    /**
     * @brief handcrufted widget to ins hexadecimal values into a QLineEdit
     */
    class DataLineInput : public HexInput
    {
        Q_OBJECT
    public:
        DataLineInput(QWidget *parent=0);
        ~DataLineInput(){};

    public slots:
        void setDataLength(int length);
    };
};

#endif /*_NDLCOM_DATALINEINPUT_H_*/

