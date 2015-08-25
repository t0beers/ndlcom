#ifndef NDLCOMBRIDGEEXTERNALINTERFACE_H
#define NDLCOMBRIDGEEXTERNALINTERFACE_H

#include "ndlcom/Bridge.h"

#include <string>

// for "speed_t"
#include <termios.h>
#include <iostream>
// for "struct sockaddr_in" and "socklen_t"
#include <arpa/inet.h>

class NDLComBridgeExternalInterface {
  public:
    NDLComBridgeExternalInterface(NDLComBridge &_bridge);
    virtual ~NDLComBridgeExternalInterface();

    static void writeWrapper(void *context, const void *buf,
                             const size_t count);
    static size_t readWrapper(void *context, void *buf, const size_t count);

    virtual void writeEscapedBytes(const void *buf, size_t count) = 0;
    virtual size_t readEscapedBytes(void *buf, size_t count) = 0;

  private:
    NDLComBridge &bridge;
    struct NDLComExternalInterface external;
};

class NDLComBridgeStream : public NDLComBridgeExternalInterface {
  public:
    NDLComBridgeStream(NDLComBridge &_bridge);
    ~NDLComBridgeStream();

  protected:
    FILE *fd_read;
    FILE *fd_write;

    size_t readEscapedBytes(void *buf, size_t count);
    void writeEscapedBytes(const void *buf, size_t count);
};

class NDLComBridgeSerial : public NDLComBridgeStream {
  public:
    NDLComBridgeSerial(NDLComBridge &_bridge, std::string device_name,
                       speed_t baudrate);
    ~NDLComBridgeSerial();

  private:
    struct termios oldtio;
    int fd;
};

class NDLComBridgeFpga : public NDLComBridgeStream {
  public:
    NDLComBridgeFpga(NDLComBridge &_bridge,
                     std::string device_name = "/dev/NDLCom");
    ~NDLComBridgeFpga();

  private:
    int fd;
};

class NDLComBridgeUdp : public NDLComBridgeExternalInterface {
  public:
    NDLComBridgeUdp(NDLComBridge &_bridge, std::string hostname,
                    unsigned int in_port, unsigned int out_port);
    ~NDLComBridgeUdp();

    size_t readEscapedBytes(void *buf, size_t count);
    void writeEscapedBytes(const void *buf, size_t count);

  private:
    struct sockaddr_in addr_in;
    struct sockaddr_in addr_out;
    int fd;
    // hmpf... the call to "recvfrom()" wants a _pointer_ to the
    // length-argument... and the pointer cannot even be a const-one...
    // manman... serious? why?
    socklen_t len;
};

#endif /*NDLCOMBRIDGEEXTERNALINTERFACE_H*/
