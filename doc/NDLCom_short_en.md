![ndlcom_logo](ndlcom_logo.png)

# NDLCom

Modern robotic systems come with a variety of decentralized sensor- and controlelektronics which frequently need to communicate with one another. The NDLCom protocol allows to exchange small packets of data between microcontrollers, FPGAs and a computer. Each participant is connected to one of its neighbours via a point-to-point connection and has to forward incoming messages according to the receiver address in the packet header. Implementations to receive, forward and decode messages exist in C/C++ and VHDL. The visualization, logging and exporting of received data is accomplished in a graphical user interfaces.
