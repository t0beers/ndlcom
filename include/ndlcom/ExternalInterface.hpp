#ifndef NDLCOMBRIDGEEXTERNALINTERFACE_H
#define NDLCOMBRIDGEEXTERNALINTERFACE_H

#include "ndlcom/Bridge.h"
#include "ndlcom/ExternalInterfaceBase.hpp"

#include <string>
// for "speed_t":
#include <termios.h>
// for "struct sockaddr_in" and "socklen_t":
#include <arpa/inet.h>

namespace ndlcom {

/**
 * @brief base-class to handle interfaces which use "FILE" internally
 *
 * this class just implements to read/write functions around the calls to
 * "fread()" and "fwrite()", as this is always (tm) the same. adds
 * error-checking and looping-until-all-bytes-are-written.
 */
class ExternalInterfaceStream : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfaceStream(
        NDLComBridge &_bridge,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceStream();

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
class NDLComBridgeSerial : public ExternalInterfaceStream {
  public:
    NDLComBridgeSerial(NDLComBridge &_bridge, std::string device_name,
                       speed_t baudrate,
                       uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
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
 */
class NDLComBridgeFpga : public ExternalInterfaceStream {
  public:
    NDLComBridgeFpga(NDLComBridge &_bridge,
                     std::string device_name = "/dev/NDLCom",
                     uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
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
class NDLComBridgeUdp : public ndlcom::ExternalInterfaceBase {
  public:
    NDLComBridgeUdp(NDLComBridge &_bridge, std::string hostname,
                    unsigned int in_port, unsigned int out_port,
                    uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
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
 * @brief transport data through "named pipe" in hex-encoded strings
 *
 * for example "0x04 0x5e 0x00 0xfd"
 *
 * by default this creates two pipes with the given
 * "pipename" as base and "_rx"/"_tx" appended. if no full path is given
 * (starting with "/") the two pipes will be created in the local folder.
 *
 * will also remove the created pipes _iff_ they where not present before!
 */
class ExternalInterfacePipe : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfacePipe(
        NDLComBridge &_bridge, std::string pipename,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfacePipe();

    size_t readEscapedBytes(void *buf, size_t count);
    void writeEscapedBytes(const void *buf, size_t count);

  private:
    FILE *str_in;
    FILE *str_out;
    bool unlinkRxPipeInDtor;
    bool unlinkTxPipeInDtor;
    std::string pipename_rx;
    std::string pipename_tx;
};

/**
 * create a pseudo terminal (eg: "pty") interface to be used for binary serial
 * port emulation.
 *
 * tries to create a named symlink in tmp pointing to the actual device node
 */
class ExternalInterfacePty : public ExternalInterfaceStream {
  public:
    ExternalInterfacePty(
        NDLComBridge &_bridge, std::string _symlinkname,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfacePty();

    size_t readEscapedBytes(void *buf, size_t count);
    // we can reuse the write function of the base-class

  private:
    // the master-side of the pty
    int pty_fd;
    std::string symlinkname;

    // if the symlink given is exising but dead we will recreate it pointing to
    // the correct new location
    void prepareSymlink() const;
    // try to delete the symlink we created before
    void cleanSymlink() const;
};
}

#endif /*NDLCOMBRIDGEEXTERNALINTERFACE_H*/
