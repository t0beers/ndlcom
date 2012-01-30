/**
 * @file NDLCom/include/NDLCom/udpcom_connect_dialog.h
 * @author Martin Zenzes, Peter Kampmann, Armin
 */

#ifndef NDLCOM_UDPCOM_CONNECT_DIALOG_H
#define NDLCOM_UDPCOM_CONNECT_DIALOG_H

#include <QDialog>
#include <QSettings>

/**
 * @addtogroup Communication
 * @{
 * @addtogroup Communication_NDLCom
 * @{
 * @addtogroup Communication_NDLCom_UDPcom UDPCom
 * @{
 */

namespace NDLCom
{
    /* forward declaration to avoid #include */
    namespace Ui { class UdpComConnectDialog; };

/**
 * @brief Connect dialog...
 */
class UdpComConnectDialog : public QDialog
{
    Q_OBJECT
public:
    UdpComConnectDialog(QDialog* parent=0);
    ~UdpComConnectDialog();

    void loadCommonConnectionSettings();
    QString getHostname();
    int getRecvport();
    int getSendport();

public slots:
    void writeCommonConnectionSettings();

private:
    Ui::UdpComConnectDialog* mpUi;
};

}; //of namespace

/**
 * @}
 * @}
 * @}
 */

#endif
