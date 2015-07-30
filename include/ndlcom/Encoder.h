/**
 * @file include/ndlcom/Encoder.h
 * @date 2011
 */

#ifndef NDLCOM_ENCODER_H
#define NDLCOM_ENCODER_H

#include "ndlcom/Types.h"

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Encode packet for serial transmission.
 *
 * On the output-buffer-size: 2bytes for start/stop-flags, the header itself,
 * the actual data with an decoded maximum of 255bytes and the CRC. Since each
 * byte (except the start/stop flags) _could_ be escaped in theory, we need
 * to double this number...
 *
 *    (2 + 2 * (sizeof(NDLComHeader) + pHeader->mDataLen + sizeof(NDLComCrc)))
 *
 * @param pOutputBuffer Data will be written into this buffer.
 * @param outputBufferSize Size of the buffer.
 * @param pHeader Pointer to a PacketHeader struct.
 * @param pData User data inside the packet.
 *              Keep in mind to set "pHeader->mDataLen" correctly!
 *              (@see struct NDLComHeader)
 *
 * @return Number of bytes used in the output buffer. 0 on too-small-buffer.
 */
size_t ndlcomEncode(void *pOutputBuffer, const size_t outputBufferSize,
                    const struct NDLComHeader *pHeader, const void *pData);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_ENCODER_H*/
