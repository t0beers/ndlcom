#ifndef NDLCOM_EXTERNALINTERFACE_HPP
#define NDLCOM_EXTERNALINTERFACE_HPP

#include "ndlcom/Bridge.h"
#include "ndlcom/ExternalInterfaceBase.hpp"

#include <string>
// for "speed_t":
#include <sys/termios.h>
// for "struct sockaddr_in" and "socklen_t":
#include <arpa/inet.h>
// detecting closed interfaces
#include <poll.h>

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
        struct NDLComBridge &_bridge, std::string label,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceStream();

  protected:
    FILE *fd_read;
    FILE *fd_write;

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

  private:
    /* used to detect when the underlying device vanishes. like usb-ports */
    struct pollfd ufd;
};

/**
 * reading and writing on a serial port with the given baudrate
 *
 * straightforward.
 */
class ExternalInterfaceSerial : public ExternalInterfaceStream {
  public:
    ExternalInterfaceSerial(
        struct NDLComBridge &_bridge, std::string device_name, speed_t baudrate,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceSerial();

  private:
    /**
     * this class tries to be a good citizen: it restores the terminal setting
     * which where there before it changed them
     */
    struct termios oldtio;
    int fd;
};

/**
 * use NDLCom-fpga-kernel module, usable on ZynqBrain for example
 *
 * this is basically a device where encoded messages can be read and written
 * to.
 *
 * @see https://git.hb.dfki.de/zynq-kernel-modules/driver-ndlcom
 */
class ExternalInterfaceFpga : public ExternalInterfaceStream {
  public:
    ExternalInterfaceFpga(
        struct NDLComBridge &_bridge, std::string device_name = "/dev/NDLCom",
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceFpga();

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
class ExternalInterfaceUdp : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfaceUdp(
        struct NDLComBridge &_bridge, std::string hostname,
        unsigned int in_port, unsigned int out_port,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceUdp();

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

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
 * essentially (only) a tcp-listener
 *
 * need some server-implementation to "listen()" before we can connect to it.
 * closes connection (with throw) on error, like unreachable destination.
 *
 * Note: the call to "connect()" is done in a blocking manner!
 */
class ExternalInterfaceTcpClient : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfaceTcpClient(
        struct NDLComBridge &_bridge, std::string hostname, unsigned int port,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceTcpClient();

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

  private:
    struct sockaddr_in addr;
    int fd;
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
 *
 * FIXME: it is easly possible to create to ExternalInterfacePipe with the same
 * symlink-name. this stems from the fact that existing pipes are reused... is
 * that this a good idea?
 */
class ExternalInterfacePipe : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfacePipe(
        struct NDLComBridge &_bridge, std::string pipename,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfacePipe();

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

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
        struct NDLComBridge &bridge, std::string symlinkname,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfacePty();

    size_t readEscapedBytes(void *buf, size_t count) override;
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

#endif /*NDLCOM_EXTERNALINTERFACE_HPP*/
