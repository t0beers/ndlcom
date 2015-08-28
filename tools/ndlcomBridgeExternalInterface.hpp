#ifndef NDLCOMBRIDGEEXTERNALINTERFACE_H
#define NDLCOMBRIDGEEXTERNALINTERFACE_H

#include "ndlcom/Bridge.h"

#include <string>

// for "speed_t"
#include <termios.h>
#include <iostream>
// for "struct sockaddr_in" and "socklen_t"
#include <arpa/inet.h>

/**
 * virtual base-class to wrap "struct NDLComExternalInterface" into a cpp-class
 *
 * stores private reference of the NDLComBridge this interface is connected to.
 */
class NDLComBridgeExternalInterface {
  public:
    NDLComBridgeExternalInterface(NDLComBridge &_bridge, uint8_t flags = 0);
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
    NDLComBridgeStream(NDLComBridge &_bridge, uint8_t flags = 0);
    ~NDLComBridgeStream();

  protected:
    FILE *fd_read;
    FILE *fd_write;

    size_t readEscapedBytes(void *buf, size_t count);
    void writeEscapedBytes(const void *buf, size_t count);
};

/**
 * reading and writing on a serial port with the given baudrate
 *
 * straightforward.
 */
class NDLComBridgeSerial : public NDLComBridgeStream {
  public:
    NDLComBridgeSerial(NDLComBridge &_bridge, std::string device_name,
                       speed_t baudrate, uint8_t flags = 0);
    ~NDLComBridgeSerial();

  private:
    struct termios oldtio;
    int fd;
};

/**
 * use NDLCom-fpga-kernel module, usable on ZynqBrain for example.
 *
 * this is basically a device where encoded messages can be read and written
 * to.
 *
 * NOTE: this still needs to be tested on real hardware. but should work, it's
 * a straight-forward interface.
 *
 * note that we do not allow any flags here...
 */
class NDLComBridgeFpga : public NDLComBridgeStream {
  public:
    NDLComBridgeFpga(NDLComBridge &_bridge,
                     std::string device_name = "/dev/NDLCom");
    ~NDLComBridgeFpga();

  private:
    int fd;
};

/**
 * passing messages through an udp interface
 *
 * tcp is not far away, should be "easy" to add.
 *
 * has distinct send- and receive-ports to allows usage on localhost.
 */
class NDLComBridgeUdp : public NDLComBridgeExternalInterface {
  public:
    NDLComBridgeUdp(NDLComBridge &_bridge, std::string hostname,
                    unsigned int in_port, unsigned int out_port,
                    uint8_t flags = 0);
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

/**
 * outputs data on a unix "named pipe" in hex-encoded form ("0x04" and so on)
 * input is also possible.
 */
class NDLComBridgeNamedPipe : public NDLComBridgeExternalInterface {
  public:
    NDLComBridgeNamedPipe(NDLComBridge &_bridge, std::string pipename,
                          uint8_t flags = 0);
    ~NDLComBridgeNamedPipe();

    size_t readEscapedBytes(void *buf, size_t count);
    void writeEscapedBytes(const void *buf, size_t count);

  private:
    FILE *str_in;
    FILE *str_out;
};

#endif /*NDLCOMBRIDGEEXTERNALINTERFACE_H*/
