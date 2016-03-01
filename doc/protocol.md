![ndlcom_header](ndlcom_header.png)

# Start Flag

The special byte `0x7e` denotes the beginning of a packet.

# Header

The simple header consists of four bytes:

- `receiverId`. Note that a receiver of `0xff` is used for Broadcast messages
- `senderId`. To be filled with the Id of the sender
- `packetCtr`. Can be used to monitor the connection qualitiy. To be
  increased for packages going from _deviceA_ to _deviceB_ for each packet sent
  from _deviceA_ to _deviceB_, wraps at 255.
- `dataLen`, describing how many bytes of payload follow

# Payload

Upto 255 bytes, as many as noted in `dataLen`.

# Checksum

A `CRC-16 AUG-CCITT`, see also [crc](crc.md).

# Stop Flag

Again, a `0x7e`. Note that the closing flag is not mandatory.
