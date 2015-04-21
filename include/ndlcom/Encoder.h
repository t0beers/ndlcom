/**
 * @file include/ndlcom/Encoder.h
 * @date 2011
 */

#ifndef NDLCOM_ENCODER_H
#define NDLCOM_ENCODER_H

#include "ndlcom/Header.h"

#include <stddef.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief Encode packet for serial transmission.
 *
 * @param pOutputBuffer Data will be written into this buffer.
 * @param outputBufferSize Size of the buffer.
 * @param pHeader Pointer to a PacketHeader struct.
 * @param pData User data inside the packet.
 *              Keep in mind to set pHeader->mDataLen!
 *              (@see struct NDLComHeader)
 *
 * @return Number of bytes used in the output buffer. -1 on error.
 */
size_t ndlcomEncode(void *pOutputBuffer, size_t outputBufferSize,
                    const NDLComHeader *pHeader, const void *pData);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_ENCODER_H*/
