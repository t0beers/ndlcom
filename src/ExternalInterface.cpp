#include "ndlcom/ExternalInterface.hpp"

#include <stdexcept>
#include <cstring>
#include <errno.h>
#include <cstdio>

// serial:
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>

// udp:
#include <netdb.h>

// pty
#include <stdlib.h>
#include <poll.h>
#include <sys/select.h>
#include <linux/limits.h>

NDLComBridgeStream::NDLComBridgeStream(NDLComBridge &_bridge, uint8_t flags)
    : ndlcom::ExternalInterfaceBase(_bridge, flags), fd_read(NULL),
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
        if (ferror(fd_read)) {
            throw std::runtime_error(strerror(ferror(fd_read)));
        } else {
            return 0;
        }
    }
    /* printf("stream read %lu bytes\n", bytesRead); */
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
    fflush(fd_write);
    /* printf("stream wrote %lu bytes\n", count); */
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
    // save oldtio
    int rc = tcgetattr(fd, &oldtio);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
    // exclusive access
    rc = ioctl(fd, TIOCEXCL);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
    // prepare newtio
    struct termios newtio = oldtio;
    cfmakeraw(&newtio);
    // no waittimes
    newtio.c_cc[VMIN] = 0;
    newtio.c_cc[VTIME] = 0;
    // set speed
    rc = cfsetspeed(&newtio, baudrate);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
    // flush (ie: discard old) the port and set new settings
    rc = tcflush(fd, TCIOFLUSH);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
    rc = tcsetattr(fd, TCSANOW, &newtio);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }

    fd_read = fdopen(fd, "r");
    if (!fd_read) {
        throw std::runtime_error(strerror(errno));
    }
    fd_write = fdopen(fd, "r+");
    if (!fd_write) {
        throw std::runtime_error(strerror(errno));
    }
}

NDLComBridgeSerial::~NDLComBridgeSerial() {
    // release exclusive access
    ioctl(fd, TIOCNXCL);
    // restore old settings.
    tcsetattr(fd, TCSANOW, &oldtio);
    close(fd);
}

NDLComBridgeFpga::NDLComBridgeFpga(NDLComBridge &_bridge,
                                   std::string device_name)
    : NDLComBridgeStream(_bridge) {
    // does non-blocking read work?
    fd = open(device_name.c_str(), O_RDWR | O_NDELAY);
    if (fd == -1) {
        throw std::runtime_error(strerror(errno));
    }
    fd_read = fdopen(fd, "r");
    if (!fd_read) {
        throw std::runtime_error(strerror(errno));
    }
    fd_write = fdopen(fd, "r+");
    if (!fd_write) {
        throw std::runtime_error(strerror(errno));
    }
}

NDLComBridgeFpga::~NDLComBridgeFpga() { close(fd); }

NDLComBridgeUdp::NDLComBridgeUdp(NDLComBridge &_bridge, std::string hostname,
                                 unsigned int in_port, unsigned int out_port,
                                 uint8_t flags)
    : ndlcom::ExternalInterfaceBase(_bridge, flags),
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
    printf("NDLComBridgeUdp: opened udp connection, reading from '%s:%d' and "
           "sending to '%s:%d'\n",
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
    if (addr_out.sin_addr.s_addr != addr_recv.sin_addr.s_addr) {
        std::string address_from(inet_ntoa(addr_out.sin_addr));
        std::string address_to(inet_ntoa(addr_recv.sin_addr));
        printf("NDLComBridgeUdp: switch outgoing connection from '%s:%d' to "
               "'%s:%d'\n",
               address_from.c_str(), ntohs(addr_out.sin_port),
               address_to.c_str(), ntohs(addr_out.sin_port));
        addr_out.sin_addr = addr_recv.sin_addr;
        /*
         * this will tell the socket to use the port of the sender upon the
         * next reply...
         *
         * NOTE: if you re-enable this line, adopt the printf above!
         */
        /* addr_out.sin_port = addr_recv.sin_port; */
    }
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
        if (written == -1) {
            if (errno == EINTR) {
                // ignore signals
                goto again;
            } else if (errno == EPIPE) {
                // this means the connection is not set up correctly... assume
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
    : ndlcom::ExternalInterfaceBase(_bridge, flags), unlinkRxPipeInDtor(false),
      unlinkTxPipeInDtor(false), pipename_rx(pipename + "_rx"),
      pipename_tx(pipename + "_tx") {

    struct stat status_in;
    struct stat status_out;

    int fd_in = open(pipename_rx.c_str(), O_RDWR | O_NDELAY);
    if (fd_in == -1) {
        // fifo might not be there yet, try again
        if (mkfifo(pipename_rx.c_str(), 0644) != 0) {
            throw std::runtime_error(strerror(errno));
        }
        // remember to delete the pipe later
        unlinkRxPipeInDtor = true;
        fd_in = open(pipename_rx.c_str(), O_RDWR | O_NDELAY);
        if (fd_in == -1) {
            throw std::runtime_error(strerror(errno));
        }
    }
    if (fstat(fd_in, &status_in) == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if (!S_ISFIFO(status_in.st_mode))
        throw std::runtime_error(pipename_rx + " is not a fifo?");

    int fd_out = open(pipename_tx.c_str(), O_RDWR | O_NDELAY);
    if (fd_out == -1) {
        if (mkfifo(pipename_tx.c_str(), 0644) != 0) {
            throw std::runtime_error(strerror(errno));
        }
        // remember to delete the pipe later
        unlinkTxPipeInDtor = true;
        fd_out = open(pipename_tx.c_str(), O_RDWR | O_NDELAY);
        if (fd_out == -1) {
            throw std::runtime_error(strerror(errno));
        }
    }
    if (fstat(fd_out, &status_out) == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if (!S_ISFIFO(status_out.st_mode))
        throw std::runtime_error(pipename_tx + " is not a fifo?");

    // convert the file-descriptors to streams
    str_in = fdopen(fd_in, "r");
    if (!str_in) {
        throw std::runtime_error(strerror(errno));
    }
    str_out = fdopen(fd_out, "r+");
    if (!str_out) {
        throw std::runtime_error(strerror(errno));
    }
}

NDLComBridgeNamedPipe::~NDLComBridgeNamedPipe() {
    // calling "fclose" will close the underlying fd as well
    fclose(str_in);
    fclose(str_out);
    // we do only delete the pipes on exit if we created them!
    if (unlinkRxPipeInDtor) {
        // printf("unlinking pipe '%s'\n", pipename_rx.c_str());
        unlink(pipename_rx.c_str());
    }
    if (unlinkTxPipeInDtor) {
        // printf("unlinking pipe '%s'\n", pipename_tx.c_str());
        unlink(pipename_tx.c_str());
    }
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
            /* std::cerr << "found nothing, destroyed: " << t << "\n"; */
        } else if (scanRet == 1) {
            /* std::cerr << "jo, did read: 0x" << std::hex << byte << std::dec
             */
            /*           << " from " << amount << "bytes\n"; */
            ((uint8_t *)buf)[readSoFar++] = (uint8_t)byte;
        } else {
            throw std::runtime_error("huh? I'm lost");
        }
    }

    /* std::cerr << "all in all, got " << readSoFar << "bytes\n"; */

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

NDLComBridgePty::NDLComBridgePty(NDLComBridge &_bridge,
                                 std::string _symlinkname, uint8_t flags)
    : NDLComBridgeStream(_bridge, flags), symlinkname(_symlinkname) {

    int rc;
    // request a new pseudoterminal
    //
    // NOTE: could use "openpty()" to wrap a number of the following
    // systemcalls... but this needs us to link against "-lutil"...
    pty_fd = posix_openpt(O_RDWR | O_NOCTTY);
    if (pty_fd < 0) {
        throw std::runtime_error(strerror(errno));
    }
    // grant slaves access
    rc = grantpt(pty_fd);
    if (rc != 0) {
        throw std::runtime_error(strerror(errno));
    }
    // allow slave access
    rc = unlockpt(pty_fd);
    if (rc != 0) {
        throw std::runtime_error(strerror(errno));
    }

    // O_NONBLOCKing access as usual
    fcntl(pty_fd, F_SETFL, O_NONBLOCK);

    // this sets the HUP flag on the tty master, used to detect a reader
    // present. see "readerPresent()" down below
    close(open(ptsname(pty_fd), O_RDWR | O_NOCTTY));

    // provide a nice symlink pointing to our "/dev/pts/\d\+"
    prepareSymlink();

    // fdopen for the base-class
    fd_read = fdopen(pty_fd, "r");
    if (!fd_read) {
        throw std::runtime_error(strerror(errno));
    }
    fd_write = fdopen(pty_fd, "r+");
    if (!fd_write) {
        throw std::runtime_error(strerror(errno));
    }

    printf(
        "NDLComBridgePty: the slave side is named '%s', the symlink is '%s'\n",
        ptsname(pty_fd), symlinkname.c_str());
}

NDLComBridgePty::~NDLComBridgePty() {
    // delete the previously created symlink
    cleanSymlink();
    // not sure...
    close(pty_fd);
}

/**
 * this function is complete overkill ;-)
 */
void NDLComBridgePty::cleanSymlink() const {
    int rc;
    struct stat buf;
    rc = lstat(symlinkname.c_str(), &buf);
    if (rc != 0) {
        throw std::runtime_error(strerror(errno));
    }
    // the "symlinkname" is a symlink...
    char symlinktargetname[PATH_MAX + 1] = {0};
    // checking where it points to:
    rc = readlink(symlinkname.c_str(), symlinktargetname, buf.st_size + 1);
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
    if (std::string(symlinktargetname) != std::string(ptsname(pty_fd))) {
        throw std::runtime_error(
            "unmatching symlink '" + symlinkname + "', pointing to '" +
            std::string(symlinktargetname) + "' instead of '" +
            std::string(ptsname(pty_fd)) + "'");
    }
    rc = unlink(symlinkname.c_str());
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
}

/** symlink handling: we will delete an existing but dead symlink to recreate
 * it, but we will bail out on anything else */
void NDLComBridgePty::prepareSymlink() const {
    int rc;
    struct stat buf;
    rc = lstat(symlinkname.c_str(), &buf);
    if (rc != 0) {
        if (errno != ENOENT) {
            throw std::runtime_error("proposed symlinkname '" + symlinkname +
                                     "' present but not a symlink");
        }
    } else {
        // the "symlinkname" is a symlink...
        char symlinktargetname[PATH_MAX + 1] = {0};
        // checking where it points to:
        rc = readlink(symlinkname.c_str(), symlinktargetname, buf.st_size + 1);
        if (rc == -1) {
            if (errno == EINVAL) {
                throw std::runtime_error("proposed symlinkname '" +
                                         symlinkname +
                                         "' present but not a symlink");
            }
        }
        // and obtain information about its target:
        rc = stat(symlinktargetname, &buf);
        // now: IFF the symlink is invlid we may delete it and continue...
        if (rc != 0) {
            throw std::runtime_error("given symlink '" + symlinkname +
                                     "' is still pointing to '" +
                                     std::string(symlinktargetname) + "'");
        } else {
            printf("NDLComBridgePty: symlink '%s' was pointing to '%s' but is "
                   "dead now, recreating\n",
                   symlinkname.c_str(), symlinktargetname);
        }
        // delete the old symlink:
        rc = unlink(symlinkname.c_str());
        if (rc == -1) {
            throw std::runtime_error(strerror(errno));
        }
    }
    // and finally: create a new symlink pointing to our pty:
    rc = symlink(ptsname(pty_fd), symlinkname.c_str());
    if (rc == -1) {
        throw std::runtime_error(strerror(errno));
    }
}
