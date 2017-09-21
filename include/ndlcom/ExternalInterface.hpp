#ifndef NDLCOM_EXTERNALINTERFACE_HPP
#define NDLCOM_EXTERNALINTERFACE_HPP

// for "struct sockaddr_in" and "socklen_t":
#include <arpa/inet.h>
#include <netinet/in.h>
// detecting closed interfaces
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <asm-generic/termbits.h>
#include <regex>
#include <string>

// ouh...
#include <linux/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>

#include "ndlcom/ExternalInterface.h"
#include "ndlcom/ExternalInterfaceBase.hpp"

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
    ~ExternalInterfaceStream() override;

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
        struct NDLComBridge &_bridge, std::string device_name,
        speed_t baudrate = defaultBaudrate,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceSerial() override;

    // regex for "uri" string, optional routing table at the end
    static const std::regex uri;
    static const speed_t defaultBaudrate;
    // ctor using provided match-argument of the given uri. we pass the
    // match-object so that we do not have a ctor accepting just a string --
    // which may be the uri or only the "ptyname"
    ExternalInterfaceSerial(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

  private:
    /**
     * this class tries to be a good citizen: it restores the terminal setting
     * which where there before it changed them
     */
    struct termios2 oldtio;
    int fd;
};

// like this? not needed right now, but would be fun...
template <class Of, class... V> struct UriHelper {
    UriHelper(std::regex _uri, V... args) : uri(_uri), defaultValues(args...){};
    typedef std::tuple<std::string, V...> tupleType;
    const std::regex uri;
    const std::tuple<V...> defaultValues;
    // why cling to "tuple"? pack expansion!
    Of generate(struct NDLComBridge &bridge, std::smatch match,
                uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT) {
        // then template-iterate the entries in "tupleType"... extract them,
        // return fully constructed new object?
    }
    Of internal(struct NDLComBridge &bridge, std::string device, V... args,
                uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT) {
        // has to use the move ctor!
        return Of(bridge, device, args..., flags);
    }
    // stuff this return value into ctor...
    tupleType fillFromMatch(std::smatch match) {
        // bla...
        //
        // fill "V..." using stringstream from "match[2 to sizeof(V...)+2]" or
        // the respective default value. pass the expanded tuple to the actual
        // ctor of the class
        //
        // needs a templated ctor which accepts and unpacks a tuple? too much
        // template magic for me right now... or invoke the other way around:
        // pass any ctor into this function, and apply template magic? like a
        // static factory function...
        //
        // this is a demo-impl for "serial":
        return tupleType(match[1], match[2].length()
                                       ? std::stoi(match[2])
                                       : std::get<1>(defaultValues));
    }
};
// then just putting in the regex and default values into the ctor if the helper
struct UriHelperSerial : UriHelper<ndlcom::ExternalInterfaceSerial, speed_t> {
    UriHelperSerial(std::regex uri, speed_t b)
        : UriHelper<ndlcom::ExternalInterfaceSerial, speed_t>(uri, b){};
};
// should be static const... cannot be part of the class!
const UriHelperSerial h = UriHelperSerial(
    std::regex("^serial://([^:&]*)(?::(\\d+))?(?:&(.*))?$"), 115200);


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
    ~ExternalInterfaceFpga() override;

    static const std::regex uri;
    ExternalInterfaceFpga(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

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
        unsigned int in_port = defaultInPort,
        unsigned int out_port = defaultOutPort,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceUdp() override;

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

    static const std::regex uri;
    static const unsigned int defaultInPort;
    static const unsigned int defaultOutPort;
    ExternalInterfaceUdp(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

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
    ~ExternalInterfaceTcpClient() override;

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

    static const std::regex uri;
    static const unsigned int defaultPort;
    ExternalInterfaceTcpClient(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

  private:
    struct sockaddr_in addr;
    int fd;
};

/**
 * see https://www.kernel.org/doc/Documentation/networking/can.txt
 * and https://github.com/linux-can/can-utils/blob/master/candump.c
 *
 * remember to bring the interface up before trying to use it:
 *
       ip link set up can0 type can bitrate 500000
       ip link set can0 txqueuelen 1000
 *
 * kernel queuing needs to be known: https://stackoverflow.com/a/43988554/7374642
 *
 * the uri accepted for this type of interface has the format
 * "can://$deviceName:canIdRx:canIdTx", where the device name is followed by
 * the two canIds used to send and receive frames.
 *
 * Additional notes:
 *
 * would be nice to register also for CAN error message, check them when
 * receiving? throw runtime-error when hardware defect is detected? or just
 * close? dunno...  see include/uapi/linux/can/error.h
 *
 * there are error frames and overload frames
 *
 * longer messages, the "can fd" type, where not considered. can be added, for
 * sure. but is not yet.
 *
 * there is a 1microsecond timestamp generated when receiving a frame. sadly we
 * cannot use it in this framework in a sensible way.
 *
 * i would prefer to use a low-priority canId because we'll use this here as
 * tunnel for debug-tooling with "legacy" ndlcom stuff...
 *
 * TODO: do our fpga-based senders create packets with less than eight byte of payload?
 */
class ExternalInterfaceCan : public ndlcom::ExternalInterfaceBase {
  public:
    ExternalInterfaceCan(
        struct NDLComBridge &_bridge, std::string device_name, canid_t canIdTx, canid_t canidRx,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);
    ~ExternalInterfaceCan() override;

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

    // could this be made into a more generic template-struct with std::tuple
    // for the default-arguments...?
    static const std::regex uri;
    static const canid_t defaultCanIdRx;
    static const canid_t defaultCanIdTx;

    // TODO: document this crap...
    ExternalInterfaceCan(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

  private:
    struct can_filter can_filter;
    canid_t canIdTx;
    canid_t canIdRx;
    can_err_mask_t err_mask;
    struct sockaddr_can addr;
    int fd;
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
    ~ExternalInterfacePipe() override;

    size_t readEscapedBytes(void *buf, size_t count) override;
    void writeEscapedBytes(const void *buf, size_t count) override;

    static const std::regex uri;
    ExternalInterfacePipe(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

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
    ~ExternalInterfacePty() override;

    size_t readEscapedBytes(void *buf, size_t count) override;
    // we can reuse the write function of the base-class

    static const std::regex uri;
    ExternalInterfacePty(
        struct NDLComBridge &_bridge, std::smatch match,
        uint8_t flags = NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT);

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
} // namespace ndlcom

#endif /*NDLCOM_EXTERNALINTERFACE_HPP*/
