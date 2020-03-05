/**
 * TODO:
 *
 * - SA_RESTART http://www.gnu.org/software/libc/manual/html_node/Flags-for-Sigaction.html
 * - add message queue https://www.cs.cf.ac.uk/Dave/C/node25.html
 */
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <netinet/ip.h>

#include "ndlcom/ExternalInterface.hpp"

#include <linux/can/error.h>

using namespace ndlcom;

ExternalInterfaceStream::ExternalInterfaceStream(struct NDLComBridge &bridge,
                                                 std::string _label,
                                                 uint8_t flags)
    : ndlcom::ExternalInterfaceBase(bridge, _label, std::cerr, flags),
      fd_read(nullptr), fd_write(nullptr) {
    // setting "udf.events" implicitly to zero means: listen only to the
    // error-events POLLHUP, POLLERR, and POLLNVAL
    memset(&ufd, 0, sizeof(struct pollfd));
}

ExternalInterfaceStream::~ExternalInterfaceStream() {
    if (fd_write) {
        fclose(fd_write);
    }
    if (fd_read) {
        fclose(fd_read);
    }
}

size_t ExternalInterfaceStream::readEscapedBytes(void *buf, size_t count) {
    // this should never happen...
    if (!fd_read) {
        return 0;
    }

    // check if our descriptor is still valid
    ufd.fd = fileno(fd_read);
again:
    if (poll(&ufd, 1, 0) < 0) {
        // cope with signals
        if (errno == EINTR) {
            goto again;
        }
    }
    // if there are any bits set in "ufd.revents", an error occured
    if (ufd.revents) {
        reportRuntimeError("connection closed itself", __FILE__, __LINE__);
        return 0;
    }

    size_t bytesRead = fread(buf, sizeof(char), count, fd_read);
    if (bytesRead == 0) {
        if (ferror(fd_read)) {
            if (errno == EAGAIN) {
                return 0;
            } else {
                reportRuntimeError("error during fread(): " +
                                       std::string(strerror(errno)),
                                   __FILE__, __LINE__);
                return 0;
            }
        } else {
            return 0;
        }
    }
    /* out << "stream read " << bytesRead << " bytes\n"; */
    return bytesRead;
}

void ExternalInterfaceStream::writeEscapedBytes(const void *buf, size_t count) {
    /* out << "trying to write " << count << "\n"; */
    if (!fd_write)
        return;
    size_t written = fwrite((const char *)buf, sizeof(char), count, fd_write);
    // happens when there is a "slow" interface which is getting data from a
    // "fast" one. it cannot cope.
    if (written != count) {
        out << label << ": bytes lost. slow interface?\n";
    }
    // not sure: flushing needed?
    fflush(fd_write);

    return;
}

ExternalInterfaceSerial::ExternalInterfaceSerial(struct NDLComBridge &bridge,
                                                 std::string device_name,
                                                 speed_t baudrate,
                                                 uint8_t flags)
    : ExternalInterfaceStream(bridge, "serial://" + device_name + ":" +
                                          std::to_string(baudrate),
                              flags) {
    fd = open(device_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // save oldtio
    int rc = ioctl(fd, TCGETS2, &oldtio);
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // exclusive access. prevents interleaving reads
    rc = ioctl(fd, TIOCEXCL);
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    /**
     * on custom-baudrate support: pretty much a horrible state...
     *
     * - open debian bugreport, 2013: https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=683826
     * - "baudrate aliasing" based on old termios (what vinzenz did): http://stackoverflow.com/a/7152671/7374642
     * - code-snippet for the idead used here: https://www.downtowndougbrown.com/2013/11/linux-custom-serial-baud-rates/
     * - mini-program with the same idea: https://gist.githubusercontent.com/sentinelt/3f1a984533556cf890d9/raw/8a35958138b1167fce5c2301a73e2fe06aeb08d8/gistfile1.c
     */

    // prepare newtio
    struct termios2 newtio = oldtio;
    // this block is copied verbatim from the manpage for "cfmakeraw" function.
    // it is dealing with a "struct termios", actually a subset of the newer
    // "struct termios2". so we could cast the pointer (uha...) or just issue
    // the same settings.
    newtio.c_iflag &=
        ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    newtio.c_oflag &= ~OPOST;
    newtio.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    newtio.c_cflag &= ~(CSIZE | PARENB);
    newtio.c_cflag |= CS8;

    // setting custom baudrate using an integer:
    newtio.c_cflag &= ~CBAUD;
    newtio.c_cflag |= BOTHER;
    newtio.c_ispeed = baudrate;
    newtio.c_ospeed = baudrate;

    // activate set new settings:
    rc = ioctl(fd, TCSETS2, &newtio);
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // finally use the obtained filedescriptior to fill the FILE things used by
    // the baseclass.
    fd_read = fdopen(fd, "r");
    if (!fd_read) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    fd_write = fdopen(fd, "r+");
    if (!fd_write) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
}

const speed_t ndlcom::ExternalInterfaceSerial::defaultBaudrate = 921600;
const std::regex ndlcom::ExternalInterfaceSerial::uri(
    "^serial://([^:&]*)(?::(\\d+))?(?:&(.*))?$");
ExternalInterfaceSerial::ExternalInterfaceSerial(struct NDLComBridge &_bridge,
                                                 std::smatch match,
                                                 uint8_t flags)
    : ExternalInterfaceSerial(_bridge, match[1],
                              match[2].length() ? std::stoi(match[2].str())
                                                : defaultBaudrate,
                              flags) {}

ExternalInterfaceSerial::~ExternalInterfaceSerial() {
    // release exclusive access
    ioctl(fd, TIOCNXCL);
    // restore old settings.
    ioctl(fd, TCSETS2, &oldtio);
    close(fd);
}

ExternalInterfaceFpga::ExternalInterfaceFpga(struct NDLComBridge &bridge,
                                             std::string device_name,
                                             uint8_t flags)
    : ExternalInterfaceStream(bridge, "fpga://" + device_name) {
    if (flags != NDLCOM_EXTERNAL_INTERFACE_FLAGS_DEFAULT) {
        reportRuntimeError("fpga module does only support default flags?",
                           __FILE__, __LINE__);
    }
    // does non-blocking read work?
    fd = open(device_name.c_str(), O_RDWR | O_NDELAY);
    if (fd == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    fd_read = fdopen(fd, "r");
    if (!fd_read) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    fd_write = fdopen(fd, "r+");
    if (!fd_write) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
}

const std::regex
    ndlcom::ExternalInterfaceFpga::uri("^fpga://([^:&]*)(?:&:(.*))?$");
ExternalInterfaceFpga::ExternalInterfaceFpga(struct NDLComBridge &_bridge,
                                             std::smatch match, uint8_t flags)
    : ExternalInterfaceFpga(_bridge, match[1], flags) {}

ExternalInterfaceFpga::~ExternalInterfaceFpga() { close(fd); }

ExternalInterfaceUdp::ExternalInterfaceUdp(struct NDLComBridge &bridge,
                                           std::string hostname,
                                           unsigned int in_port,
                                           unsigned int out_port,
                                           unsigned int socket_priority, uint8_t flags)
    : ndlcom::ExternalInterfaceBase(bridge, "udp://" + hostname + ":" +
                                                std::to_string(in_port) + ":" +
                                                std::to_string(out_port) +
                                                " Socket priority: " +
                                                std::to_string(socket_priority),
                                    std::cerr, flags),
      len(sizeof(struct sockaddr_in)) {

    // prevent this:
    if (in_port == out_port) {
        reportRuntimeError("inport and outport are the same", __FILE__,
                           __LINE__);
    }
    
    // "AF_INET" for ipv4-only
    struct addrinfo hints = {0};
    // conversion to ipv6 needs more than changing this... also convert all
    // calls from "inet_ntoa()" to "inet_ntop()"
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    // create the socket (which is a file descriptor):
    fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol);
    if (fd == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // non-blocking access
    if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    unsigned int option_value = 0;
    socklen_t option_len = sizeof(option_value);
    
    if(setsockopt(fd, SOL_SOCKET, SO_PRIORITY, (void *)&socket_priority, sizeof(socket_priority)) == -1) {
	reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    else {
	// check if the socket priority level was set properly
	getsockopt(fd, SOL_SOCKET, SO_PRIORITY, (void *)&option_value, &option_len);
	out << "Set UDP socket priority to:" << option_value << "\n"; 
	if (option_value != socket_priority)
	{
	  out << "Setting socket priority failed\n";
	}
    }
    
    // NOTE: the structure returned into this pointer needs to be freed later!
    struct addrinfo *result;
    // try to resolve the hostname-string
    if (int retval =
            getaddrinfo(hostname.c_str(), nullptr, &hints, &result) != 0) {
        reportRuntimeError(gai_strerror(retval), __FILE__, __LINE__);
    }

    // FIXME: if multiple results are provided: how to choose?
    /* int ctr = 0; */
    /* for (struct addrinfo *rp = result; rp != nullptr; rp = rp->ai_next) { */
    /*     out << "hostname " << ctr++ << ": '" << hostname.c_str() */
    /*         << "' resolved to '" */
    /*         << inet_ntoa(((struct sockaddr_in *)rp->ai_addr)->sin_addr) */
    /*         << "'\n"; */
    /* } */

    // copy the resulting address to "addr_in" variable
    memcpy(&addr_in, (struct sockaddr_in *)result->ai_addr,
           sizeof(struct sockaddr_in));
    addr_in.sin_port = htons(in_port);
    // we will receive from any adress. otherwise we would have to own the
    // address in question. which we only do for local ones?
    addr_in.sin_addr.s_addr = INADDR_ANY;

    // same for "addr_out"...s
    memcpy(&addr_out, (struct sockaddr_in *)result->ai_addr,
           sizeof(struct sockaddr_in));
    addr_out.sin_port = htons(out_port);

    // and then bind the resulting "in" address to the actual socket. now we
    // will receive from this address -- UDP, form any IP and the given port
    if (bind(fd, (struct sockaddr *)&addr_in, sizeof(struct sockaddr_in)) ==
        -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // "inet_ntoa()" does have an internal char-array, we cannot call it twice
    // during assembling the output string. whould be better to use
    // "inet_ntop()", but hey...
    std::string address_in(inet_ntoa(addr_in.sin_addr));
    std::string address_out(inet_ntoa(addr_out.sin_addr));
    /* out << "ExternalInterfaceUdp: opened udp connection, reading from '" */
    /*     << address_in.c_str() << ":" << ntohs(addr_in.sin_port) */
    /*     << "' and sending to '" << address_out.c_str() << ":" */
    /*     << ntohs(addr_out.sin_port) << "'\n"; */

    // clean the shit up
    freeaddrinfo(result);
}

const unsigned int ndlcom::ExternalInterfaceUdp::defaultInPort = 34000;
const unsigned int ndlcom::ExternalInterfaceUdp::defaultOutPort = 34001;
const unsigned int ndlcom::ExternalInterfaceUdp::defaultSocketPriority = 0;
const std::regex ndlcom::ExternalInterfaceUdp::uri(
    "^udp://([^:&]*)(?::(\\d+))?(?::(\\d+))?(?::(\[0-9]|1[0-5]))?(?:&(.*))?$");
ExternalInterfaceUdp::ExternalInterfaceUdp(struct NDLComBridge &_bridge,
                                           std::smatch match, uint8_t flags)
    : ExternalInterfaceUdp(
          _bridge, match[1],
          match[2].length() ? std::stoi(match[2].str()) : defaultInPort,
          match[3].length() ? std::stoi(match[3].str()) : defaultOutPort,
	  match[4].length() ? std::stoi(match[4].str()) : defaultSocketPriority,
          flags) {}

ExternalInterfaceUdp::~ExternalInterfaceUdp() { close(fd); }

size_t ExternalInterfaceUdp::readEscapedBytes(void *buf, size_t count) {
    /* out << "trying to read " << count << " bytes\n"; */
    struct sockaddr_in addr_recv;
again:
    ssize_t bytesRead =
        recvfrom(fd, buf, count, 0, (struct sockaddr *)&addr_recv, &len);
    if (bytesRead < 0) {
        if (errno == EINTR) {
            // ignore signals
            goto again;
        } else if (errno == EAGAIN) {
            // nothing to read, just return
            return 0;
        } else if (errno == ENOTCONN) {
            // "not connected" may happen on tcp-streams. ignore this, we just
            // assume that we know what we do...
            return 0;
        }
        // TODO: is this correct?
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        return 0;
    }
    /* out << "read " << bytesRead << " bytes from '" */
    /*     << inet_ntoa(addr_recv.sin_addr) << ":" << ntohs(addr_recv.sin_port)
     */
    /*     << "'\n"; */
    if (addr_out.sin_addr.s_addr != addr_recv.sin_addr.s_addr) {
        std::string address_from(inet_ntoa(addr_out.sin_addr));
        std::string address_to(inet_ntoa(addr_recv.sin_addr));
        out << "ExternalInterfaceUdp: switch outgoing connection from '"
            << address_from.c_str() << ":" << ntohs(addr_out.sin_port)
            << "' to '" << address_to.c_str() << ":" << ntohs(addr_out.sin_port)
            << "'\n";
        addr_out.sin_addr = addr_recv.sin_addr;
        /*
         * this will tell the socket to use the port of the sender upon the
         * next reply...
         *
         * NOTE: if you re-enable this line, adopt the output above!
         */
        /* addr_out.sin_port = addr_recv.sin_port; */
    }
    return bytesRead;
}

void ExternalInterfaceUdp::writeEscapedBytes(const void *buf, size_t count) {
    /* out << "trying to write " << count << " bytes\n"; */
    size_t alreadyWritten = 0;
again:
    while (alreadyWritten < count) {
        ssize_t written =
            sendto(fd, (const char *)buf + alreadyWritten,
                   count - alreadyWritten, MSG_NOSIGNAL,
                   (struct sockaddr *)&addr_out, sizeof(struct sockaddr_in));
        if (written == -1) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            } else if (errno == EPIPE) {
                // this means the connection is not set up correctly... assume
                // that we know what we do...
                return;
            }
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
        /* out << "wrote " << written << " bytes to '" */
        /*     << inet_ntoa(addr_out.sin_addr) << ":" <<
         * ntohs(addr_out.sin_port) */
        /*     << "'\n"; */
        alreadyWritten += written;
    }
    return;
}

ExternalInterfaceCan::ExternalInterfaceCan(struct NDLComBridge &bridge,
                                           std::string device_name,
                                           canid_t _canIdRx, canid_t _canIdTx,
                                           uint8_t flags)
    : ndlcom::ExternalInterfaceBase(bridge, "can://" + device_name + ":" +
                                                std::to_string(_canIdRx) + ":" +
                                                std::to_string(_canIdTx),
                                    std::cerr, flags),
      /* filtering for a certain canId mask */
      can_filter{_canIdRx, CAN_SFF_MASK},
      /* store the value nevertheless, for future reference: */
      canIdTx(_canIdTx), canIdRx(_canIdRx),
      /* actually errors would be handy as well (untested): */
      err_mask(CAN_ERR_TX_TIMEOUT | CAN_ERR_CRTL_RX_PASSIVE |
               CAN_ERR_CRTL_TX_PASSIVE | CAN_ERR_BUSOFF),
      addr{0}, len(sizeof(struct sockaddr_in)) {

    // error is missing/not working as expected

    // create the socket (which is a file descriptor):
    fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // non-blocking access? error will return "-1", which is true in this if-block
    if (fcntl(fd, F_SETFL, O_NONBLOCK)) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // add name resolution:
    struct ifreq ifr;

    // the "device_name" shall contain the equivalent string of smth like "can0"
    strcpy(ifr.ifr_name, device_name.data());
    if (ioctl(fd, SIOCGIFINDEX, &ifr)) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
// what the hell are these for?
#if 0
    addr.can_addr.tp.tx_id = _canIdTx;
    addr.can_addr.tp.rx_id = _canIdRx;
#endif

    bind(fd, (struct sockaddr *)&addr, sizeof(addr));

    // proper error detection... relax as needed!
    if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_ERR_FILTER, &err_mask,
                   sizeof(err_mask)) == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // and only receive these can-messages which match the filter
    if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &can_filter,
                   sizeof(can_filter)) == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // debugging, so that we see any packets at all:
#if 0
    int loopback = 1; /* 0 = disabled, 1 = enabled (default) */
    setsockopt(fd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof(loopback));
    int recv_own_msgs = 1; /* 0 = disabled (default), 1 = enabled */
    setsockopt(fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs,
               sizeof(recv_own_msgs));
#endif
}

const canid_t ndlcom::ExternalInterfaceCan::defaultCanIdRx = 42;
const canid_t ndlcom::ExternalInterfaceCan::defaultCanIdTx = 43;
const std::regex ndlcom::ExternalInterfaceCan::uri(
    "^can://([^:&]*)(?::(\\d+))?(?::(\\d+))?(?:&(.*))?$");
ExternalInterfaceCan::ExternalInterfaceCan(struct NDLComBridge &_bridge,
                                           std::smatch match, uint8_t flags)
    : ExternalInterfaceCan(
          _bridge, match[1],
          match[2].length() ? std::stoi(match[2].str()) : defaultCanIdRx,
          match[3].length() ? std::stoi(match[3].str()) : defaultCanIdTx,
          flags) {}

ExternalInterfaceCan::~ExternalInterfaceCan() { close(fd); }

size_t ExternalInterfaceCan::readEscapedBytes(void *buf, size_t count) {
    struct can_frame frame;

    size_t alreadyRead = 0;
    // this loop will only read multiples of the frame-size into count. the
    // last read of a non-modulo-eight sized packet is chopped off?
    while (alreadyRead < (count - sizeof(frame.data))) {
again:
        // using recvfrom, so that we specifiy the interface from where we read
        ssize_t bytesRead = recvfrom(fd, &frame, sizeof(struct can_frame), 0,
                                     (struct sockaddr *)&addr, &len);
        if (bytesRead < 0) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            } else if (errno == EAGAIN) {
                // nothing more to read, just return
                goto done;
            }
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
            return 0;
        }
        if (bytesRead < sizeof(struct can_frame)) {
            reportRuntimeError("verybogus", __FILE__, __LINE__);
            return 0;
        }

        // TODO: if interface would be bound to "any" we would have to add
        // additional checks?

        memcpy((char*)buf+alreadyRead, frame.data, frame.can_dlc);

        alreadyRead += frame.can_dlc;
    }
done:

    return alreadyRead;
}

void ExternalInterfaceCan::writeEscapedBytes(const void *buf, size_t count) {
    out << "trying to write " << count << " bytes\n";
    size_t alreadyWritten = 0;
    struct can_frame frame;
    // the "sockaddr_can" struct has a rx/tx field
    frame.can_id = canIdTx;
    frame.can_dlc = sizeof(frame.data);
again:
    while (alreadyWritten < count) {

        // we'll rewrite the "size" field only for the last chunk. for all the
        // other ones the initial value (8byte, set before the loop) is used.
        // it might work like this?
        if ((alreadyWritten+sizeof(frame.data)) > count) {
            frame.can_dlc = count - alreadyWritten;
        }

        memcpy(frame.data, (const char *)buf + alreadyWritten, frame.can_dlc);

        std::cout << "here: " << (int)frame.can_dlc << " " << sizeof(frame.data) << "\n";

        ssize_t written = sendto(fd, &frame, sizeof(struct can_frame), 0,
                                 (struct sockaddr *)&addr, sizeof(addr));
        if (written == -1) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            }
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
        alreadyWritten += frame.can_dlc;
    }
    return;
}

ExternalInterfaceTcpClient::ExternalInterfaceTcpClient(
    struct NDLComBridge &bridge, std::string hostname, unsigned int port,
    uint8_t flags)
    : ndlcom::ExternalInterfaceBase(bridge, "tcpclient://" + hostname + ":" +
                                                std::to_string(port),
                                    std::cerr, flags) {

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if ((fd = socket(hints.ai_family, hints.ai_socktype, hints.ai_protocol)) < 0) {
        reportRuntimeError("failed to create socket: " +
                               std::string(strerror(errno)),
                           __FILE__, __LINE__);
    }

    struct addrinfo *result;
    // try to resolve the hostname-string
    if (int retval =
            getaddrinfo(hostname.c_str(), nullptr, &hints, &result) != 0) {
        reportRuntimeError(gai_strerror(retval), __FILE__, __LINE__);
    }

    memset(&addr, 0, sizeof(addr));
    memcpy(&addr, (struct sockaddr_in *)result->ai_addr,
           sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    /* establish connection */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        reportRuntimeError("failed to connect: " + std::string(strerror(errno)),
                           __FILE__, __LINE__);
    }
    // only after calling "connect()" we switch to nonblocking mode. would have
    // to wait anyways... this blocks the gui during "connect()", but...
    fcntl(fd, F_SETFL, O_NONBLOCK);

    freeaddrinfo(result);
}

const unsigned int ndlcom::ExternalInterfaceTcpClient::defaultPort = 2000;
const std::regex ndlcom::ExternalInterfaceTcpClient::uri(
    "^tcpclient://([^:&]*)(?::(\\d+))?(?:&(.*))?$");
ExternalInterfaceTcpClient::ExternalInterfaceTcpClient(
    struct NDLComBridge &_bridge, std::smatch match, uint8_t flags)
    : ExternalInterfaceTcpClient(
          _bridge, match[1],
          match[2].length() ? std::stoi(match[2].str()) : defaultPort, flags) {}

ExternalInterfaceTcpClient::~ExternalInterfaceTcpClient() { close(fd); }

size_t ExternalInterfaceTcpClient::readEscapedBytes(void *buf, size_t count) {
again:
    ssize_t bytesRead = recv(fd, buf, count, 0);
    if (bytesRead < 0) {
        if (errno == EINTR) {
            // ignore signals
            goto again;
        } else if (errno == EAGAIN) {
            // nothing to read, just return
            return 0;
        }
        // TODO: is this correct?
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        return 0;
    }
    return bytesRead;
}

void ExternalInterfaceTcpClient::writeEscapedBytes(const void *buf,
                                                   size_t count) {
    size_t alreadyWritten = 0;
again:
    while (alreadyWritten < count) {
        ssize_t written = send(fd, (const char *)buf + alreadyWritten,
                               count - alreadyWritten, MSG_NOSIGNAL);
        if (written == -1) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            }
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
        alreadyWritten += written;
    }
    return;
}

ExternalInterfacePipe::ExternalInterfacePipe(struct NDLComBridge &bridge,
                                             std::string pipename,
                                             uint8_t flags)
    : ndlcom::ExternalInterfaceBase(bridge, "pipe://" + pipename, std::cerr,
                                    flags),
      unlinkRxPipeInDtor(false), unlinkTxPipeInDtor(false),
      pipename_rx(pipename + "_rx"), pipename_tx(pipename + "_tx") {
    if (pipename.empty()) {
        reportRuntimeError("no pipename given", __FILE__, __LINE__);
    }

    struct stat status_in;
    struct stat status_out;

    // now follows a more complicated block, determining if the intended pipes
    // are already present. if so, we do not need to create nor delete them.
    int fd_in = open(pipename_rx.c_str(), O_RDWR | O_NDELAY);
    if (fd_in == -1) {
        // fifo might not be there yet, try again
        if (mkfifo(pipename_rx.c_str(), 0644) != 0) {
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
        // remember to delete the pipe later
        unlinkRxPipeInDtor = true;
        fd_in = open(pipename_rx.c_str(), O_RDWR | O_NDELAY);
        if (fd_in == -1) {
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
    }
    if (fstat(fd_in, &status_in) == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    if (!S_ISFIFO(status_in.st_mode)) {
        reportRuntimeError(pipename_rx + " is not a fifo?", __FILE__, __LINE__);
    }

    int fd_out = open(pipename_tx.c_str(), O_RDWR | O_NDELAY);
    if (fd_out == -1) {
        if (mkfifo(pipename_tx.c_str(), 0644) != 0) {
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
        // remember to delete the pipe later
        unlinkTxPipeInDtor = true;
        fd_out = open(pipename_tx.c_str(), O_RDWR | O_NDELAY);
        if (fd_out == -1) {
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
    }
    if (fstat(fd_out, &status_out) == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    if (!S_ISFIFO(status_out.st_mode)) {
        reportRuntimeError(pipename_tx + " is not a fifo?", __FILE__, __LINE__);
    }

    // convert the file-descriptors to streams
    str_in = fdopen(fd_in, "r");
    if (!str_in) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    str_out = fdopen(fd_out, "r+");
    if (!str_out) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
}

const std::regex
    ndlcom::ExternalInterfacePipe::uri("^pipe://([^:&]*)(?:&(.*))?$");
ExternalInterfacePipe::ExternalInterfacePipe(struct NDLComBridge &_bridge,
                                             std::smatch match, uint8_t flags)
    : ExternalInterfacePipe(_bridge, match[1], flags) {}

ExternalInterfacePipe::~ExternalInterfacePipe() {
    // calling "fclose" will close the underlying fd as well
    fclose(str_in);
    fclose(str_out);
    // we do only delete the pipes on exit if we created them!
    if (unlinkRxPipeInDtor) {
        /* out << "unlinking pipe '" << pipename_rx << "'\n"; */
        unlink(pipename_rx.c_str());
    }
    if (unlinkTxPipeInDtor) {
        /* out << "unlinking pipe '" << pipename_tx << "'\n"; */
        unlink(pipename_tx.c_str());
    }
}

size_t ExternalInterfacePipe::readEscapedBytes(void *buf, size_t count) {
    // wanna accept strings like "0xff 0x88..." and convert them to raw bytes
    // into the provided char-array, which is returned to the caller for
    // parsing. non-blocking!

    size_t readSoFar = 0;
    unsigned int byte = 0;
    int amount = -1;
    while (readSoFar < count) {
        int scanRet = fscanf(str_in, " 0x%02x%n", &byte, &amount);

        if (scanRet == EINTR) {
            // ignore signals (sigpipe comes to mind)
            continue;
        }
        if (scanRet == EOF) {
            break;
        } else if (scanRet == 0) {
            // could not read anything... discard some bytes. TODO: this should
            // be doable more efficiently...
            char t[2];
            // should not return EOF, "fscanf()" should have handled this
            char *ret = fgets(t, 2, str_in);
            // error or EOF...? honestly, I don't care
            if (!ret) {
                reportRuntimeError("fgets didn't zer0?", __FILE__, __LINE__);
                // never return negative, in case reportRuntimeError is
                // overloaded to _not_ throw
                return 0;
            }
            /* std::cerr << "found nothing, destroyed: " << t << "\n"; */
        } else if (scanRet == 1) {
            /* std::cerr << "jo, did read: 0x" << std::hex << byte << std::dec
             */
            /*           << " from " << amount << "bytes\n"; */
            ((uint8_t *)buf)[readSoFar++] = (uint8_t)byte;
        } else {
            reportRuntimeError("huh? I'm lost", __FILE__, __LINE__);
        }
    }

    /* std::cerr << "all in all, got " << readSoFar << "bytes\n"; */

    return readSoFar;
}

void ExternalInterfacePipe::writeEscapedBytes(const void *buf, size_t count) {
    // simple
    for (size_t i = 0; i < count; ++i) {
        // printing hex-encoded packets into the pipe we opened before
        fprintf(str_out, "0x%02x ", (int)((uint8_t *)buf)[i]);
    }

    fprintf(str_out, "\n");
    // writing to pipes is not soo reliable... do not fully understand what is
    // going on there...
    fflush(str_out);

    return;
}

ExternalInterfacePty::ExternalInterfacePty(struct NDLComBridge &bridge,
                                           std::string _symlinkname,
                                           uint8_t flags)
    : ExternalInterfaceStream(bridge, "pty://" + _symlinkname, flags),
      symlinkname(_symlinkname) {
    if (symlinkname.empty()) {
        reportRuntimeError("no symlinkname for pty given", __FILE__, __LINE__);
    }

    int rc;
    // request a new pseudoterminal
    //
    // NOTE: could use "openpty()" to wrap a number of the following
    // systemcalls... but this needs us to link against "-lutil"...
    pty_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_fd < 0) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // grant slaves access
    rc = grantpt(pty_fd);
    if (rc != 0) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // allow slave access
    rc = unlockpt(pty_fd);
    if (rc != 0) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    // O_NONBLOCKing access as usual
    fcntl(pty_fd, F_SETFL, O_NONBLOCK);

    // provide a nice symlink pointing to our "/dev/pts/\d\+"
    prepareSymlink();

    // fdopen for the base-class
    fd_read = fdopen(pty_fd, "r");
    if (!fd_read) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    fd_write = fdopen(pty_fd, "r+");
    if (!fd_write) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }

    out << "ExternalInterfacePty: the slave side is named '" << ptsname(pty_fd)
        << "', the symlink is '" << symlinkname << "'\n";
}

const std::regex
    ndlcom::ExternalInterfacePty::uri("^pty://([^:&]*)(?:&(.*))?$");
ExternalInterfacePty::ExternalInterfacePty(struct NDLComBridge &_bridge,
                                           std::smatch match, uint8_t flags)
    : ExternalInterfacePty(_bridge, match[1], flags) {}

ExternalInterfacePty::~ExternalInterfacePty() {
    // delete the previously created symlink
    cleanSymlink();
    // not sure...
    close(pty_fd);
}

/**
 * this function is complete overkill ;-)
 *
 * FIXME:
 * - somehow i could create two pty with the same symlink. it seemed to work a
 *   bit?
 * - there is also some funny receiving-with-noone-connected going on...
 */
void ExternalInterfacePty::cleanSymlink() const {
    int rc;
    struct stat buf;
    rc = lstat(symlinkname.c_str(), &buf);
    if (rc != 0) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    // the "symlinkname" is a symlink...
    char symlinktargetname[PATH_MAX + 1] = {0};
    // checking where it points to:
    rc = readlink(symlinkname.c_str(), symlinktargetname, buf.st_size + 1);
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
    if (std::string(symlinktargetname) != std::string(ptsname(pty_fd))) {
        reportRuntimeError(
            "unmatching symlink '" + symlinkname + "', pointing to '" +
                std::string(symlinktargetname) + "' instead of '" +
                std::string(ptsname(pty_fd)) + "'",
            __FILE__, __LINE__);
    }
    rc = unlink(symlinkname.c_str());
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
}

/**
 * symlink handling: we will delete an existing but dead symlink to recreate
 * it, but we will bail out on anything else
 */
void ExternalInterfacePty::prepareSymlink() const {
    int rc;
    struct stat buf;
    rc = lstat(symlinkname.c_str(), &buf);
    if (rc != 0) {
        if (errno != ENOENT) {
            reportRuntimeError("proposed symlinkname '" + symlinkname +
                                   "' present but not a symlink",
                               __FILE__, __LINE__);
        }
    } else {
        // the "symlinkname" is a symlink...
        char symlinktargetname[PATH_MAX + 1] = {0};
        // checking where it points to:
        rc = readlink(symlinkname.c_str(), symlinktargetname, buf.st_size + 1);
        if (rc == -1) {
            if (errno == EINVAL) {
                reportRuntimeError("proposed symlinkname '" + symlinkname +
                                       "' present but not a symlink",
                                   __FILE__, __LINE__);
            }
        }
        // and obtain information about its target:
        rc = stat(symlinktargetname, &buf);
        // now: IFF the symlink is invlid we may delete it and continue...
        if (rc != 0) {
            reportRuntimeError("given symlink '" + symlinkname +
                                   "' is still pointing to '" +
                                   std::string(symlinktargetname) + "'",
                               __FILE__, __LINE__);
        } else {
            out << "ExternalInterfacePty: symlink '" << symlinkname
                << "' was pointing to '" << symlinktargetname
                << "' but is dead now, recreating\n";
        }
        // delete the old symlink:
        rc = unlink(symlinkname.c_str());
        if (rc == -1) {
            reportRuntimeError(strerror(errno), __FILE__, __LINE__);
        }
    }
    // and finally: create a new symlink pointing to our pty:
    rc = symlink(ptsname(pty_fd), symlinkname.c_str());
    if (rc == -1) {
        reportRuntimeError(strerror(errno), __FILE__, __LINE__);
    }
}

size_t ExternalInterfacePty::readEscapedBytes(void *buf, size_t count) {
    if (!fd_read) {
        return 0;
    }
    size_t bytesRead = fread(buf, sizeof(char), count, fd_read);
    if (bytesRead == 0) {
        if (ferror(fd_read)) {
            if (errno == EAGAIN || errno == EIO) {
                // in case of a pty, slaves connecting and disconnecting are
                // seen as "errors", but we need to ignore some of them
                return 0;
            } else {
                reportRuntimeError("error during fread(): " +
                                       std::string(strerror(errno)),
                                   __FILE__, __LINE__);
                return 0;
            }
        } else {
            return 0;
        }
    }
    return bytesRead;
}
