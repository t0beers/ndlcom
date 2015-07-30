/**
 * @file include/ndlcom/Parser.h
 * @date 2011
 */

#ifndef NDLCOM_PARSER_H
#define NDLCOM_PARSER_H

#include "ndlcom/Types.h"

#include <stddef.h>

/**
 * @brief C Implementation of the core parser for the NDLCom protocol.
 *
 * This code is intended to be built into a library, which then in turn may be
 * linked into your binary.
 *
 * To use, include the header-file:
 * @code
 *  #include "ndlcom/Parser.h"
 * @endcode
 *
 * initialize the basic struct NDLComParserState by doing
 * @code
 *      uint8_t buffer[sizeof(NDLComParsrer)];
 *      NDLComParser* pNDLComParser = ndlcomParserCreate(buffer, sizeof(buffer));
 * @endcode
 *
 * To transmit at the very lowest level, create a packet by doing
 * @code
 *      hdr.mDataLen = sizeof(struct data);
 *      hdr.mReceiverId = 0x01;
 *      hdr.mSenderId = 0x02;
 *      hdr.mCounter++;
 *      size_t bufferLength = ndlcomEncode( (void*)bufferTx,
 *                                          sizeof(bufferTx),
 *                                          &hdr,
 *                                          (const void*)(&data),
 *                                          sizeof(struct data) );
 * @endcode
 *
 * And send the resulting bytes using your usart-routine.
 *
 * Note that receiving needs initializing with ndlcomParserCreate(), while
 * ndlcomEncode() is working on-the-fly using a different buffer.
 *
 * @see struct NDLComHeader
 *
 * @section com1 Compiling
 *
 * To compile this code, it is advised to use the provided CMake-structure
 * based on pkg_config files. This allows easy inclusion for complete
 * in-source-builds. Alternatively it is possible to "install" the files in a
 * specific directory and reuse them manually.
 *
 * @code
 *  $ make # build natively
 *  $ make install # build and install natively
 * @endcode
 *
 * For cross-compiling it is possible to use CMake's toolchain files, resident
 * in CMake-Modules, by specifying them via make. See the makefile for adding
 * this manually.
 */

#if defined(__cplusplus)
extern "C" {
#endif

/**
 * @brief State for ndlcomParser functions.
 *
 * Since a packet may be distributed over many sequential calls of
 * ndlcomParserReceive(), a buffer and some state variables are required to
 * reconstruct the packet information.
 */
struct NDLComParser {
    /** decoded header */
    union {
        uint8_t raw[sizeof(struct NDLComHeader)];
        struct NDLComHeader hdr;
    } mHeader;
    /** Checksum of data (header + packet content) while receiving. */
    NDLComCrc mDataCRC;
    /** stores if the last received byte was a escaped byte */
    int8_t mLastWasESC;
    /**< Current write position while receiving header data. */
    uint8_t *mpHeaderWritePos;
    /** storage for a decoded payload */
    uint8_t mpData[NDLCOM_MAX_PAYLOAD_SIZE];
    /** Current write position of next data byte while receiving user data. */
    uint8_t * mpDataWritePos;
    /** different states the parser may have. */
    enum ParserState {
        mcERROR = 0,
        mcWAIT_HEADER,
        mcWAIT_DATA,
        mcWAIT_FIRST_CRC_BYTE,
        mcWAIT_SECOND_CRC_BYTE,
        mcCOMPLETE
    } mState;
    /** how often a bad crc was received */
    uint32_t mNumberOfCRCFails;
};

/**
 * @brief Create new parser state information in buffer.
 * @param pBuffer Pointer to buffer.
 * @param dataBufSize Size of buffer.
 *
 * @return 0 on error, otherwise a pointer to be used by other functions
 *     in this file.
 */
struct NDLComParser *ndlcomParserCreate(void *pBuffer,
                                        const size_t dataBufSize);

/**
 * @brief Destroy state information (before freeing the used buffer).
 *
 * Currently not needed since no "private" heap memory allocation is done.
 * @param parser Pointer to state information.
 */
void ndlcomParserDestroy(struct NDLComParser *parser);

/**
 * @brief Append as many bytes as possible from a buffer to the internal data
 * structure.
 *
 * @param parser Pointer to state information.
 * @param buf Pointer to received data that should be parsed.
 * @param buflen Number of bytes to be parsed.
 * @return number of accepted bytes
 */
size_t ndlcomParserReceive(struct NDLComParser *parser, const void *newData,
                           size_t newDataLen);

/**
 * @brief Return true if a packet is available.
 *
 * allows to be used on different parsers, since it has no internal state.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return returns true if a packet was received
 */
char ndlcomParserHasPacket(const struct NDLComParser *parser);

/**
 * @brief Return pointer to header.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return Pointer to a received protocol-header
 */
const struct NDLComHeader *ndlcomParserGetHeader(const struct NDLComParser *parser);

/**
 * @brief Return pointer to data.
 *
 * The data is available in the header. @see struct NDLComHeader
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return Pointer to the received payload. By definition of the protocol, the
 * first byte denotes the type of the payload
 */
const void *ndlcomParserGetPacket(const struct NDLComParser *parser);

/**
 * @brief Detection of telegram starts
 *
 * Different approach for forwarding. Allows cutting of data stream in sync
 * with packet-headers by looking at single bytes Simpler and hopefully faster
 * than complete parser.  May be used for forwarding of packets.
 *
 * Internal method is very similar (in fact copied) from the statemachine used
 * in ndlcomParserReceive().
 *
 * @param c The current byte in the datastream.
 * @return true is new packet detected
*/
static inline uint8_t ndlcomDetectNewPaket(const uint8_t c) {
    return c == NDLCOM_START_STOP_FLAG;
}

/**
 * @brief After using the data pointer allow the parser to receive the next
 * packet.
 *
 * @see ndlcomParserGetPacket
 *
 * @param parser Pointer to the parser state-struct to be used
 */
void ndlcomParserDestroyPacket(struct NDLComParser *parser);

/**
 * @brief can be used to get the name of the current parser-state
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return name of the current state
 */
const char* ndlcomParserGetState(const struct NDLComParser *parser);

/**
 * @brief Return the number of CRC failures.
 *
 * @param parser Pointer to the parser state-struct to be used
 * @return the number of CRC failures
 */
uint32_t ndlcomParserGetNumberOfCRCFails(const struct NDLComParser *parser);

/**
 * @brief Reset CRC failure counter.
 *
 * @param parser Pointer to the parser state-struct to be used
 */
void ndlcomParserResetNumberOfCRCFails(struct NDLComParser *parser);

#if defined(__cplusplus)
}
#endif

#endif /*NDLCOM_PARSER_H*/
