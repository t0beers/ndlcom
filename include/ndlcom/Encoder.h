/**
 * @file include/ndlcom/Encoder.h
 * @date 2011
 */

#ifndef NDLCOM_ENCODER_H
#define NDLCOM_ENCODER_H

#include <stddef.h>

struct NDLComHeader;

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Encode packet for serial transmission.
 *
 * On the output-buffer-size: 2bytes for start/stop-flags, the header itself,
 * the actual data with an decoded maximum of 256bytes and the checksum. Since
 * each byte (except the start/stop flags) _could_ be escaped in theory, we
 * need to double this number to get the worst-case number of bytes...
 *
 *    (2 + 2 * (sizeof(NDLComHeader) + pHeader->mDataLen + sizeof(NDLComCrc)))
 *
 * NOTE: error-passing of a too-small buffer is a bit broken. this function
 * will just return 0 or silently return a half-encoded message if the output
 * buffer is deemed too small.
 *
 * @param outputBuffer Data will be written into this buffer.
 * @param outputBufferSize Size of the buffer.
 * @param header Pointer to a PacketHeader struct.
 * @param data User data inside the packet. Set "header->mDataLen" correctly!
 *
 * @return Number of bytes used in the output buffer. 0 on too-small-buffer.
 */
size_t ndlcomEncode(void *outputBuffer, const size_t outputBufferSize,
                    const struct NDLComHeader *header, const void *data);

/**
 * @brief encoding header and multiple memory segments into one output buffer
 *
 * functionality very similar to the simple "ndlcomEncode()", but here a given
 * number of "void*"/"size_t" pairs can be additionally specified. for example:
 *

     struct NDLComHeader header;
     uint8_t data1[4] = {1, 2, 3, 4};
     uint8_t data2[8] = {1, 2, 3, 4, 5, 6, 7, 8};
     header.mDataLen = sizeof(data1) + sizeof(data2);

     uint8_t output[NDLCOM_MAX_ENCODED_MESSAGE_SIZE];

     size_t sz = ndlcomEncodeVar(output, sizeof(output), &header, 2, data1,
                                 sizeof(data1), data2, sizeof(data2));

 *
 * to encode the two blocks "data1" and "data2" into the "output" buffer.
 *
 * NOTE: be carefull, the compiler cannot help you with type-checking the
 * arguments following the "additionalSections". if you mess up on the argument
 * types or the number thereof, the stack will probably be broken.
 * also see http://stackoverflow.com/a/3105382/4658481
 *
 * @param outputBuffer Data will be written into this buffer.
 * @param outputBufferSize Size of the buffer.
 * @param header Pointer to a PacketHeader struct.
 * @param additionalSections number of following "void*"/"size_t" pairs
 *
 * @return Number of bytes used in the output buffer. 0 on too-small-buffer.
 */
size_t ndlcomEncodeVar(void *outputBuffer, const size_t outputBufferSize,
                       const struct NDLComHeader *header,
                       size_t additionalSections, ...);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_ENCODER_H*/
