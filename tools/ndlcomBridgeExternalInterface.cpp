#include "ndlcomBridgeExternalInterface.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>

// serial:
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

// udp:
#include <netdb.h>

NDLComBridgeExternalInterface::NDLComBridgeExternalInterface(
    NDLComBridge &_bridge, uint8_t flags)
    : bridge(_bridge) {
    ndlcomExternalInterfaceInit(
        &external, NDLComBridgeExternalInterface::writeWrapper,
        NDLComBridgeExternalInterface::readWrapper, flags, this);

    ndlcomBridgeRegisterExternalInterface(&bridge, &external);
}

NDLComBridgeExternalInterface::~NDLComBridgeExternalInterface() {
    ndlcomBridgeDeregisterExternalInterface(&bridge, &external);
}

void NDLComBridgeExternalInterface::writeWrapper(void *context, const void *buf,
                                                 const size_t count) {
    class NDLComBridgeExternalInterface *self =
        static_cast<class NDLComBridgeExternalInterface *>(context);
    self->writeEscapedBytes(buf, count);
}

size_t NDLComBridgeExternalInterface::readWrapper(void *context, void *buf,
                                                  const size_t count) {
    class NDLComBridgeExternalInterface *self =
        static_cast<class NDLComBridgeExternalInterface *>(context);
    return self->readEscapedBytes(buf, count);
}

NDLComBridgeStream::NDLComBridgeStream(NDLComBridge &_bridge, uint8_t flags)
    : NDLComBridgeExternalInterface(_bridge, flags), fd_read(NULL),
      fd_write(NULL) {}

NDLComBridgeStream::~NDLComBridgeStream() {
    if (fd_write) {
        fclose(fd_write);
    }
    if (fd_read) {
        fclose(fd_read);
    }
}

size_t NDLComBridgeStream::readEscapedBytes(void *buf, size_t count) {
    if (!fd_read) {
        return 0;
    }
    size_t bytesRead = fread(buf, sizeof(char), count, fd_read);
    if (bytesRead == 0) {
        if (feof(fd_read)) {
            return 0;
        } else {
            throw std::runtime_error(strerror(ferror(fd_read)));
        }
    }
    return bytesRead;
}

void NDLComBridgeStream::writeEscapedBytes(const void *buf, size_t count) {
    /* printf("trying to write %lu bytes\n", count); */
    if (!fd_write)
        return;
    size_t alreadyWritten = 0;
    while (alreadyWritten < count) {
        size_t written = fwrite((const char *)buf + alreadyWritten,
                                sizeof(char), count - alreadyWritten, fd_write);
        if (written == 0) {
            throw std::runtime_error(strerror(ferror(fd_write)));
        }
        alreadyWritten += written;
    }
    /* printf("wrote %lu bytes\n", alreadyWritten); */
    return;
}

NDLComBridgeSerial::NDLComBridgeSerial(NDLComBridge &_bridge,
                                       std::string device_name,
                                       speed_t baudrate, uint8_t flags)
    : NDLComBridgeStream(_bridge, flags) {
    fd = open(device_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        throw std::runtime_error(strerror(errno));
    }
    fcntl(fd, F_SETFL, 0);
    // exclusive access
    ioctl(fd, TIOCEXCL);
    // save oldtio
    tcgetattr(fd, &oldtio);
    // prepare newtio
    struct termios newtio;
    cfmakeraw(&newtio);
    newtio.c_cflag |= CLOCAL | CREAD;
    // no parity
    newtio.c_cflag &= ~(PARENB & PARODD);
    // no waittimes
    newtio.c_cc[VMIN] = 0;
    newtio.c_cc[VTIME] = 0;
    // set speed
    cfsetspeed(&newtio, baudrate);
    // flush the port and set new settings
    tcflush(fd, TCIFLUSH);
    tcsetattr(fd, TCSANOW, &newtio);

    // fdopen
    fd_read = fdopen(fd, "r");
    fd_write = fdopen(fd, "r+");
}

NDLComBridgeSerial::~NDLComBridgeSerial() {
    // restore old settings.
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
}

NDLComBridgeFpga::NDLComBridgeFpga(NDLComBridge &_bridge,
                                   std::string device_name)
    : NDLComBridgeStream(_bridge) {
    // does non-blocking read work?
    fd = open("/dev/NDLCom", O_RDWR | O_NDELAY);
    if (fd == -1) {
        throw std::runtime_error(strerror(errno));
    }
    fd_read = fdopen(fd, "r");
    fd_write = fdopen(fd, "r+");
}

NDLComBridgeFpga::~NDLComBridgeFpga() { close(fd); }

NDLComBridgeUdp::NDLComBridgeUdp(NDLComBridge &_bridge, std::string hostname,
                                 unsigned int in_port, unsigned int out_port,
                                 uint8_t flags)
    : NDLComBridgeExternalInterface(_bridge, flags),
      len(sizeof(struct sockaddr_in)) {
    // "AF_INET" for ipv4-only
    struct addrinfo hints = {0};
    // conversion to ipv6 needs more than changing this... also convert all
    // calls from "inet_ntoa()" to "inet_ntop()"
    hints.ai_family = AF_INET;
    // TODO: looks like setting this to "SOCK_STREAM" is all needed to enable
    // tcp
    hints.ai_socktype = SOCK_DGRAM;

    // create the socket (which is a file descriptor):
    fd = socket(hints.ai_family, SOCK_NONBLOCK | hints.ai_socktype, 0);
    if (fd == -1) {
        throw std::runtime_error(strerror(errno));
    }

    // NOTE: the structure returned into this pointer needs to be freed later!
    struct addrinfo *result;
    // try to resolve the hostname-string
    if (int retval =
            getaddrinfo(hostname.c_str(), NULL, &hints, &result) != 0) {
        throw std::runtime_error(gai_strerror(retval));
    }

    // FIXME: if multiple results are provided: how to choose?
    /* int ctr = 0; */
    /* for (struct addrinfo *rp = result; rp != NULL; rp = rp->ai_next) { */
    /*     printf("hostname %i: '%s' resolved to '%s'\n", ctr++,
     * hostname.c_str(), */
    /*            inet_ntoa(((struct sockaddr_in *)rp->ai_addr)->sin_addr)); */
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
        throw std::runtime_error(strerror(errno));
    }

    // "inet_ntoa()" does have an internal char-array, we cannot call it twice
    // in the printf-invocation. whould be better to use "inet_ntop()", but
    // hey...
    std::string address_in(inet_ntoa(addr_in.sin_addr));
    std::string address_out(inet_ntoa(addr_out.sin_addr));
    printf(
        "opened udp connection, reading from '%s:%d' and sending to '%s:%d'\n",
        address_in.c_str(), ntohs(addr_in.sin_port), address_out.c_str(),
        ntohs(addr_out.sin_port));

    // clean the shit up
    freeaddrinfo(result);
}

NDLComBridgeUdp::~NDLComBridgeUdp() { close(fd); }

size_t NDLComBridgeUdp::readEscapedBytes(void *buf, size_t count) {
    /* printf("trying to read\n"); */
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
        throw std::runtime_error(strerror(errno));
    }
    /* printf("read %lu bytes from '%s:%d'\n", bytesRead, */
    /*        inet_ntoa(addr_recv.sin_addr), ntohs(addr_recv.sin_port)); */
    return bytesRead;
}

void NDLComBridgeUdp::writeEscapedBytes(const void *buf, size_t count) {
    /* printf("trying to write %lu bytes\n", count); */
    size_t alreadyWritten = 0;
again:
    while (alreadyWritten < count) {
        ssize_t written =
            sendto(fd, (const char *)buf + alreadyWritten,
                   count - alreadyWritten, MSG_NOSIGNAL,
                   (struct sockaddr *)&addr_out, sizeof(struct sockaddr_in));
        if (written < 0) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            } else if (errno == EPIPE) {
                // this means the connection is not setup correctly... assume
                // that we know what we do...
                return;
            }
            throw std::runtime_error(strerror(errno));
        }
        /* printf("wrote %lu bytes to '%s:%d'\n", written, */
        /*        inet_ntoa(addr_out.sin_addr), ntohs(addr_out.sin_port)); */
        alreadyWritten += written;
    }
    return;
}

NDLComBridgeNamedPipe::NDLComBridgeNamedPipe(NDLComBridge &_bridge,
                                             std::string pipename,
                                             uint8_t flags)
    : NDLComBridgeExternalInterface(_bridge, flags) {

    struct stat status_in;
    struct stat status_out;
    std::string pipename_in = std::string(pipename + "_in");
    std::string pipename_out = std::string(pipename + "_out");

    int fd_in = open(pipename_in.c_str(), O_RDWR | O_NDELAY);
    if (fd_in == -1) {
        // fifo might not be there yet, try again
        if (mkfifo(pipename_in.c_str(), 0644) != 0) {
            throw std::runtime_error(strerror(errno));
        }
        fd_in = open(pipename_in.c_str(), O_RDWR | O_NDELAY);
        if (fd_in == -1) {
            throw std::runtime_error(strerror(errno));
        }
    }
    if (fstat(fd_in, &status_in) == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if (!S_ISFIFO(status_in.st_mode))
        throw std::runtime_error(pipename_in + " is not a fifo?");

    int fd_out = open(pipename_out.c_str(), O_RDWR | O_NDELAY);
    if (fd_out == -1) {
        if (mkfifo(pipename_out.c_str(), 0644) != 0) {
            throw std::runtime_error(strerror(errno));
        }
        fd_out = open(pipename_out.c_str(), O_RDWR | O_NDELAY);
        if (fd_out == -1) {
            throw std::runtime_error(strerror(errno));
        }
    }
    if (fstat(fd_out, &status_out) == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if (!S_ISFIFO(status_out.st_mode))
        throw std::runtime_error(pipename_out + " is not a fifo?");

    // convert the file-descriptors to streams
    str_in = fdopen(fd_in, "r");
    str_out = fdopen(fd_out, "r+");
}

NDLComBridgeNamedPipe::~NDLComBridgeNamedPipe() {
    // calling "fclose" will close the underlying fd as well
    fclose(str_in);
    fclose(str_out);
    // we do _not_ delete the pipes on exit, even if we created them!
}

size_t NDLComBridgeNamedPipe::readEscapedBytes(void *buf, size_t count) {
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
            fgets(t, 2, str_in);
            /* std::cout << "found nothing, destroyed: " << t << "\n"; */
        } else if (scanRet == 1) {
            /* std::cout << "jo, did read: 0x" << std::hex << byte << std::dec
             */
            /*           << " from " << amount << "bytes\n"; */
            ((uint8_t *)buf)[readSoFar++] = (uint8_t)byte;
        } else {
            throw std::runtime_error("huh? I'm lost");
        }
    }

    /* std::cout << "all in all, got " << readSoFar << "bytes\n"; */

    return readSoFar;
}

void NDLComBridgeNamedPipe::writeEscapedBytes(const void *buf, size_t count) {
    // simple
    for (size_t i = 0; i < count; ++i) {
        fprintf(str_out, "0x%02x ", (int)((uint8_t *)buf)[i]);
    }

    fprintf(str_out, "\n");
    // writing to pipes is not soo reliable... do not fully understand what is
    // going on there...
    fflush(str_out);

    return;
}
