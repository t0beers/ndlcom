# Checksum

Various notes used during development of the new crc16 functionality.
The old variant used a XOR-based checksum which was not good enough. It can still be enabled at compile time by removing the C-language define `NDLCOM_CRC16` in [Types.h](include/ndlcom/Crc.h#L14).

## CRC16

See [RFC-1662](https://tools.ietf.org/html/rfc1662), seems to be known as _CRC-CCITT 16bit_, actually have a look at ISO [13239:2002](http://read.pudn.com/downloads138/sourcecode/others/589576/ISO13239.pdf)

Some other links:
- [reveng](http://reveng.sourceforge.net) is very invaluable, see [reveng-1.1.4.tar.xz](http://downloads.sourceforge.net/project/reveng/1.1.4/reveng-1.1.4.tar.xz)
- [lammertbies](http://www.lammertbies.nl/comm/info/crc-calculation.html) provides a site with webapp, [c-library](http://www.lammertbies.nl/download/lib_crc.zip) and forum.
- a [tcl-implementation](http://www.cerfacs.fr/oa4web/oasis4_dev/gui/doc-doxygen/html/crc16_8tcl_source.html)
- another [javascript](http://depa.usst.edu.cn/chenjq/www2/software/crc/CRC_Javascript/CRC16calculation0b.htm) version
- some stronger [wording](http://srecord.sourceforge.net/crc16-ccitt.html) for the use of `CRC-16/AUG-CCITT`

# Human-language algorithm description

Mainly used for the C-Implementation.
Implicitly shared by the VHDL-Implementation.

## Encoding

What to do to calculate a crc while sending a packet:

- Create crc with `0x1021` polynomial
- Unescaped payload is used for crc-calculation
- Initialize new crcs with `0xffff` as start-value
- Start/stop flag and escape-flags are never seen by crc function, removed earlier
- No final xor of the crc-value
- The crc-value itself is escaped if needed and then appended after the last escaped payload-byte, before the packet-end-flag

## Decoding

Verifying the crc upon receiving a packet:

- New crc is again initialized with `0xffff`
- After de-escaping the received bytes, header+payload is stuffed into crc-function
- The crc-value itself is also passed into the function
- If after the header+payload+crc was fed into the crc-function, the result is `0x0000` on success
