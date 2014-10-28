/**
 * @file include/ndlcom/Protocol.h
 * @date 2011
 */

#ifndef NDLCOM_PROTOCOL_H
#define NDLCOM_PROTOCOL_H

#include "ndlcom/Header.h"
#include "ndlcom/ParserState.h"

/**
 * C Implentation for easy usage of the iStruct&SeeGrip NDLCom protocol.
 *
 * This code is intended to be build into a static library, which then in turn may
 * be linked into your binary. There, the makefile supports different architectures.
 *
 * To use, include the header-file:
 * @code
 *  #include "ndlcom/Protocol.h"
 * @endcode
 *
 * initialize the basic struct NDLComParserState by doing
 * @code
 *      pNDLComParser = ndlcomParserCreate(bufferRx, sizeof(bufferRx));
 * @endcode
 *
 * To transmit at the very lowest level, create a paket by doing
 * @code
 *      hdr.mDataLen = sizeof(struct data);
 *      hdr.mReceiverId = 0x01;
 *      hdr.mSenderId = 0x02;
 *      hdr.mCounter = 0;
 *      hdr.mCounter++;
 *      bufferLength = ndlcomEncode( (void*)bufferTx,
 *                                    sizeof(bufferTx),
 *                                    &hdr,
 *                                    (const void*)(&data),
 *                                    sizeof(struct data) );
 * @endcode
 * And send the resulting bytes using your usart-routine.
 *
 * It can be stressed, that receiving needs initializing with ndlcomParserCreate(), while
 * ndlcomEncode()
 * is working on-the-fly using a different(!) buffer
 *
 * @todo
 *  - make crc-counter a 16bit (or even 32?) value to have no overflows
 *
 * @see struct NDLComHeader
 *
 * @section com1 Compiling
 *
 * To compile this code, it is advised to use the provided cmake-structure based on pkg-config
 * files. this allows easy inclusion for complete in-source-builds. alternatively it is possible to
 * "install" the files in a specific directory and reuse them manually.
 *
 * @code
 *  $ make # build natively
 *  $ make instal # build and install natively
 * @endcode
 *
 * for crosscompiling it is possible to use cmake's toolchain files, resident in CMake-Modules, by
 * specifying them via make. see the makefile for adding this manually.
 *
 */

#include <stdint.h>

#if defined (__cplusplus)
extern "C" {
#endif

/* forward declaration */
struct NDLComParser;

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
int16_t ndlcomEncode(void* pOutputBuffer,
                     uint16_t outputBufferSize,
                     const NDLComHeader* pHeader,
                     const void* pData);


/**
 * @brief Create new parser state information in buffer.
 *\param pBuffer Pointer to buffer.
 *\param dataBufSize Size of buffer.
 *
 *\return 0 on error, otherwise a pointer to be used by other functions
 *     in this file.
 */
struct NDLComParser* ndlcomParserCreate(void* pBuffer, uint16_t dataBufSize);

/**
 * @brief Destroy state information (before freeing the used buffer).
 *
 * Currently not needed since no "private" heap memory allocation is done.
 *\param parser Pointer to state information.
 */
void ndlcomParserDestroy(struct NDLComParser* parser);

/**
 * @brief Set parser flags.
 *\param parser Pointer to state information.
 *\param flag Flags to be set.
 */
void ndlcomParserSetFlag(struct NDLComParser* parser, int flag);

/**
 * @brief Clear parser flags.
 *\param parser Pointer to state information.
 *\param flag Flags to be cleared.
 */
void ndlcomParserClearFlag(struct NDLComParser* parser, int flag);

/**
 * @brief Append as many bytes as possible from a buffer to the internal data structure.
 *
 *\param parser Pointer to state information.
 *\param buf Pointer to received data that should be parsed.
 *\param buflen Number of bytes to be parsed.
 *\return number of accepted bytes. -1 on error.
 */
 int16_t ndlcomParserReceive(struct NDLComParser* parser, const void* buf, uint16_t buflen);

/**
 * @brief Return true if a packet is available.
 *
 * allows to be used on different parsers, since it has no internal state.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return returns true if a paket was received
 */
char ndlcomParserHasPacket(struct NDLComParser* parser);


/**
 * @brief Return pointer to header.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return Pointer to a received protocol-header
 */
const NDLComHeader* ndlcomParserGetHeader(struct NDLComParser* parser);

/**
 * @brief Return pointer to data.
 *
 * The data is available in the header. @see struct NDLComHeader
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return Pointer to the received payload. by definition of the protocol, the first
 * byte denotes the type of the payload
 */
const void* ndlcomParserGetPacket(struct NDLComParser* parser);

/**
 * @brief Detection of telegram starts
 *
 * Different approach for forwarding. allows cutting of datastream in sync with paket-headers by
 * looking at single bytes Simpler and hopefully faster than complete parser. May be used for
 * forwarding of pakets.
 *
 * Internal method is very similar (in fact copied) from the statemachine used in ndlcomParserReceive().
 *
 * @param c The current byte in the datastream.
 * @return true is new paket detected
*/
static inline uint8_t ndlcomDetectNewPaket(const uint8_t c)
{
    return c == NDLCOM_START_STOP_FLAG;
}

/**
 * @brief After using the data pointer allow the parser to receive the next packet.
 *\see ndlcomParserGetPacket
 *
 * @param parser Pointer to the parser state-struct to be used
 */
void ndlcomParserDestroyPacket(struct NDLComParser* parser);

/**
 * @brief may be used to get the current state. usefull for displaying it.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @param output the current state of the parser get written here
 */
void ndlcomParserGetState(struct NDLComParser* parser,
                          struct NDLComParserState* output);

/**
 * @brief NULL-terminated string containing the name of the current state
 */
extern const char* ndlcomParserStateName[];

#if defined (__cplusplus)
}
#endif

#endif/*NDLCOM_PROTOCOL_H*/
