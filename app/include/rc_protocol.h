#ifndef RC_PROTOCOL_H
#define RC_PROTOCOL_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "bsp_err_sts.h"


/* =========================================================================
 * PROTOCOL CONFIGURATION
 * ========================================================================= */

/**
 * @brief Current RideCast protocol version.
 */
#define RC_PROTOCOL_VERSION    0x01U

/**
 * @brief Maximum application payload size.
 */
#define RC_MAX_PAYLOAD_LEN     4096U


/* =========================================================================
 * FRAME FORMAT
 * =========================================================================
 *
 *  +--------+--------+-----------+----------+--------+---------+--------+
 *  | HEADER | MSG ID | TRANSPORT | SEQUENCE | LENGTH | PAYLOAD | CRC16  |
 *  | 1 byte | 1 byte |  1 byte   | 2 bytes  | 2 byte | N bytes | 2 byte |
 *  +--------+--------+-----------+----------+--------+---------+--------+
 *
 *  HEADER:
 *
 *  Bit 7   6 | 5   4 | 3   2   1   0
 *  ----------+-------+---------------
 *  Version   | Type  | Flags
 *
 *  Version : 2 bits
 *  Type    : 2 bits
 *  Flags   : 4 bits
 *
 * ========================================================================= */

/* Frame field lengths */
#define RC_FRAME_HEADER_LEN    1U
#define RC_FRAME_MSG_ID_LEN    1U
#define RC_FRAME_TRANSPORT_LEN 1U
#define RC_FRAME_SEQUENCE_LEN  2U
#define RC_FRAME_LENGTH_LEN    2U
#define RC_FRAME_CRC_LEN       2U

/* Frame length before payload */
#define RC_FRAME_FIXED_LEN     7U

/* Minimum frame = fixed fields + CRC */
#define RC_FRAME_MIN_LEN       (RC_FRAME_FIXED_LEN + RC_FRAME_CRC_LEN)

/* Maximum frame = fixed fields + maximum payload + CRC */
#define RC_FRAME_MAX_LEN \
    (RC_FRAME_FIXED_LEN + RC_MAX_PAYLOAD_LEN + RC_FRAME_CRC_LEN)


/* =========================================================================
 * HEADER
 * ========================================================================= */

#define RC_HDR_VERSION_SHIFT   6U
#define RC_HDR_TYPE_SHIFT      4U
#define RC_HDR_FLAGS_SHIFT     0U

#define RC_HDR_VERSION_MASK    0xC0U
#define RC_HDR_TYPE_MASK       0x30U
#define RC_HDR_FLAGS_MASK      0x0FU


/* =========================================================================
 * CRC16
 * ========================================================================= */
#define RC_CRC16_POLYNOMIAL    0x1021U
#define RC_CRC16_INITIAL_VALUE 0xFFFFU

/* =========================================================================
 * FLAGS
 * ========================================================================= */

/**
 * @brief No protocol flags.
 */
#define RC_FLAG_NONE           0x00U


/* =========================================================================
 * MESSAGE TYPE
 * ========================================================================= */

/**
 * @brief RideCast message type.
 */
typedef enum
{
    eRcTypeCmd    = 0x00U, /**< Host -> RideCast command */
    eRcTypeStatus = 0x01U, /**< RideCast -> Host status/event */
    eRcTypeData   = 0x02U, /**< Host <-> RideCast application data */
    eRcTypeAck    = 0x03U  /**< Acknowledgement */

} rc_type_t;


/* =========================================================================
 * MESSAGE ID
 * ========================================================================= */

/**
 * @brief RideCast protocol message identifiers.
 *
 * These IDs describe the protocol operation.
 * Transport-specific configuration is not defined here.
 */
typedef enum
{
    /* ---------------------------------------------------------------------
     * Common
     * --------------------------------------------------------------------- */

    eRcMsgPing       = 0x01U,
    eRcMsgPong       = 0x02U,
    eRcMsgGetVersion = 0x03U,
    eRcMsgGetStatus  = 0x04U,
    eRcMsgReset      = 0x05U,


    /* ---------------------------------------------------------------------
     * Wi-Fi control
     * --------------------------------------------------------------------- */

    eRcMsgWifiSetConfig = 0x10U,
    eRcMsgWifiGetConfig = 0x11U,
    eRcMsgWifiStart     = 0x12U,
    eRcMsgWifiStop      = 0x13U,
    eRcMsgWifiStatus    = 0x14U,


    /* ---------------------------------------------------------------------
     * BLE control
     * --------------------------------------------------------------------- */

    eRcMsgBleSetConfig = 0x20U,
    eRcMsgBleGetConfig = 0x21U,
    eRcMsgBleStart     = 0x22U,
    eRcMsgBleStop      = 0x23U,
    eRcMsgBleStatus    = 0x24U,


    /* ---------------------------------------------------------------------
     * Bluetooth Classic control
     * --------------------------------------------------------------------- */

    eRcMsgBtClassicSetConfig = 0x30U,
    eRcMsgBtClassicGetConfig = 0x31U,
    eRcMsgBtClassicStart     = 0x32U,
    eRcMsgBtClassicStop      = 0x33U,
    eRcMsgBtClassicStatus    = 0x34U,


    /* ---------------------------------------------------------------------
     * Application data
     * --------------------------------------------------------------------- */

    eRcMsgSendData = 0x40U,


    /* ---------------------------------------------------------------------
     * Connection events
     * --------------------------------------------------------------------- */

    eRcMsgConnected        = 0x50U,
    eRcMsgDisconnected     = 0x51U,
    eRcMsgReconnecting     = 0x52U,
    eRcMsgConnectionFailed = 0x53U,


    /* ---------------------------------------------------------------------
     * Acknowledgement
     * --------------------------------------------------------------------- */

    eRcMsgAck = 0xF0U

} rc_msg_id_t;



/* =========================================================================
 * FRAME STRUCTURE
 * ========================================================================= */

/**
 * @brief Decoded RideCast protocol frame.
 *
 * The payload pointer points directly into the received buffer.
 * No memory allocation or payload copy is performed.
 */
typedef struct
{
    uint8_t version;
    uint8_t type;
    uint8_t flags;
    uint8_t messageId;
    uint8_t transport;
    uint16_t sequence;
    uint16_t payloadLen;
    const uint8_t* payload;
} rc_frame_t;


/* =========================================================================
 * HEADER CREATION
 * ========================================================================= */

/**
 * @brief Create a RideCast protocol header.
 *
 * Header format:
 *
 *   [7:6] Version
 *   [5:4] Message Type
 *   [3:0] Flags
 */
static inline uint8_t rc_make_header(uint8_t version, rc_type_t type, uint8_t flags)
{
    return (uint8_t)((((uint8_t)version << RC_HDR_VERSION_SHIFT) & RC_HDR_VERSION_MASK) |

                     (((uint8_t)type << RC_HDR_TYPE_SHIFT) & RC_HDR_TYPE_MASK) |

                     (((uint8_t)flags << RC_HDR_FLAGS_SHIFT) & RC_HDR_FLAGS_MASK));
}


/* =========================================================================
 * HEADER DECODING
 * ========================================================================= */

/**
 * @brief Get protocol version from header.
 */
static inline uint8_t rc_get_version(uint8_t header)
{
    return (uint8_t)((header & RC_HDR_VERSION_MASK) >> RC_HDR_VERSION_SHIFT);
}


/**
 * @brief Get message type from header.
 */
static inline rc_type_t rc_get_type(uint8_t header)
{
    return (rc_type_t)((header & RC_HDR_TYPE_MASK) >> RC_HDR_TYPE_SHIFT);
}


/**
 * @brief Get flags from header.
 */
static inline uint8_t rc_get_flags(uint8_t header)
{
    return (uint8_t)((header & RC_HDR_FLAGS_MASK) >> RC_HDR_FLAGS_SHIFT);
}


/* =========================================================================
 * UINT16 HELPERS
 * ========================================================================= */

/**
 * @brief Write uint16 value in big-endian format.
 */
static inline void rc_write_u16(uint8_t* buffer, uint16_t value)
{
    buffer[0U] = (uint8_t)((value >> 8U) & 0xFFU);
    buffer[1U] = (uint8_t)(value & 0xFFU);
}


/**
 * @brief Read uint16 value in big-endian format.
 */
static inline uint16_t rc_read_u16(const uint8_t* buffer)
{
    return (uint16_t)(((uint16_t)buffer[0U] << 8U) | (uint16_t)buffer[1U]);
}


/* =========================================================================
 * CRC16
 * =========================================================================
 *
 * CRC-16/CCITT
 *
 * Polynomial : 0x1021
 * Initial    : 0xFFFF
 *
 * CRC is calculated over:
 *
 *   HEADER
 *   MSG ID
 *   TRANSPORT
 *   SEQUENCE
 *   LENGTH
 *   PAYLOAD
 *
 * CRC bytes themselves are not included.
 *
 * ========================================================================= */
/**
 * @brief Calculate CRC16.
 *
 * @param[in] data   Data buffer.
 * @param[in] length Number of bytes.
 *
 * @return Calculated CRC16.
 */
static inline uint16_t rc_crc16(const uint8_t* data, uint16_t length)
{
    uint16_t crc = RC_CRC16_INITIAL_VALUE;
    uint16_t i;
    uint8_t bit;

    if((data == NULL) && (length > 0U))
    {
        return 0U;
    }

    for(i = 0U; i < length; i++)
    {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);

        for(bit = 0U; bit < 8U; bit++)
        {
            if((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1U) ^ RC_CRC16_POLYNOMIAL);
            }
            else
            {
                crc <<= 1U;
            }
        }
    }

    return crc;
}


/**
 * @brief Validate CRC16 of a complete frame.
 *
 * @param[in] frame    Complete frame including CRC.
 * @param[in] frameLen Complete frame length.
 *
 * @return true  CRC is valid.
 * @return false CRC is invalid.
 */
static inline bool rc_validate_crc(const uint8_t* frame, uint16_t frameLen)
{
    uint16_t calculatedCrc;
    uint16_t receivedCrc;

    if((frame == NULL) || (frameLen < RC_FRAME_MIN_LEN))
    {
        return false;
    }

    calculatedCrc = rc_crc16(frame, (uint16_t)(frameLen - RC_FRAME_CRC_LEN));
    receivedCrc   = rc_read_u16(&frame[frameLen - RC_FRAME_CRC_LEN]);
    return (calculatedCrc == receivedCrc);
}


/* =========================================================================
 * SIMPLE FRAME ENCODE
 * =========================================================================
 *
 * Used for messages without application payload:
 *
 *   PING
 *   PONG
 *   START
 *   STOP
 *   STATUS
 *   ACK
 *   etc.
 *
 * Frame:
 *
 *   HEADER | MSG_ID | TRANSPORT | SEQUENCE | LENGTH | CRC16
 *
 * ========================================================================= */

/**
 * @brief Encode a frame without payload.
 *
 * @param[out] buf       Output buffer.
 * @param[in]  bufLen    Output buffer size.
 * @param[in]  type      Message type.
 * @param[in]  flags     Protocol flags.
 * @param[in]  messageId Message ID.
 * @param[in]  transport Transport.
 * @param[in]  sequence  Sequence number.
 *
 * @return Complete frame length.
 * @return 0 on error.
 */
static inline uint16_t rc_encode_simple(uint8_t* buf,
                                        uint16_t bufLen,
                                        uint8_t type,
                                        uint8_t flags,
                                        uint8_t messageId,
                                        uint8_t transport,
                                        uint16_t sequence)
{
    uint16_t crc;

    if((buf == NULL) || (bufLen < RC_FRAME_MIN_LEN))
    {
        return 0U;
    }

    /* Header */
    buf[0U] = rc_make_header(RC_PROTOCOL_VERSION, type, flags);

    /* Message ID */
    buf[1U] = (uint8_t)messageId;

    /* Transport */
    buf[2U] = (uint8_t)transport;

    /* Sequence */
    rc_write_u16(&buf[3U], sequence);

    /* Payload length = 0 */
    rc_write_u16(&buf[5U], 0U);

    /* CRC */
    crc = rc_crc16(buf, RC_FRAME_FIXED_LEN);

    rc_write_u16(&buf[RC_FRAME_FIXED_LEN], crc);

    return RC_FRAME_MIN_LEN;
}


/* =========================================================================
 * DATA FRAME ENCODE
 * =========================================================================
 *
 * Application data is treated as opaque bytes.
 *
 * RideCast does not interpret the payload.
 *
 * Frame:
 *
 *   HEADER | MSG_ID | TRANSPORT | SEQUENCE | LENGTH | PAYLOAD | CRC16
 *
 * ========================================================================= */

/**
 * @brief Encode application data frame.
 *
 * @param[out] buf        Output buffer.
 * @param[in]  bufLen     Output buffer size.
 * @param[in]  transport  Wireless transport.
 * @param[in]  sequence   Sequence number.
 * @param[in]  payload    Application payload.
 * @param[in]  payloadLen Payload length.
 *
 * @return Complete frame length.
 * @return 0 on error.
 */
static inline uint16_t rc_encode_data(uint8_t* buf,
                                      uint16_t bufLen,
                                      uint8_t transport,
                                      uint16_t sequence,
                                      const uint8_t* payload,
                                      uint16_t payloadLen)
{
    uint16_t crc;
    uint16_t frameLen;
    uint16_t i;

    if((buf == NULL) || (payload == NULL) || (payloadLen == 0U) ||
       (payloadLen > RC_MAX_PAYLOAD_LEN))
    {
        return 0U;
    }

    frameLen = (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen + RC_FRAME_CRC_LEN);

    if(bufLen < frameLen)
    {
        return 0U;
    }

    /* Header */
    buf[0U] = rc_make_header(RC_PROTOCOL_VERSION, eRcTypeData, RC_FLAG_NONE);

    /* Message ID */
    buf[1U] = (uint8_t)eRcMsgSendData;

    /* Transport */
    buf[2U] = (uint8_t)transport;

    /* Sequence */
    rc_write_u16(&buf[3U], sequence);

    /* Payload length */
    rc_write_u16(&buf[5U], payloadLen);

    /* Payload */
    for(i = 0U; i < payloadLen; i++)
    {
        buf[RC_FRAME_FIXED_LEN + i] = payload[i];
    }

    /* CRC */
    crc = rc_crc16(buf, (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen));

    rc_write_u16(&buf[RC_FRAME_FIXED_LEN + payloadLen], crc);

    return frameLen;
}


/* =========================================================================
 * FRAME DECODE
 * =========================================================================
 *
 * Decodes:
 *
 *   HEADER | MSG_ID | TRANSPORT | SEQUENCE | LENGTH | PAYLOAD | CRC16
 *
 * The payload is not copied.
 *
 * frame->payload points directly into rawBuf.
 *
 * ========================================================================= */

/**
 * @brief Decode and validate a complete RideCast frame.
 *
 * @param[in]  rawBuf Raw received frame.
 * @param[in]  rawLen Number of bytes received.
 * @param[out] frame  Decoded frame.
 *
 * @return RET_ERR_STS_OK             Frame is valid.
 * @return RET_ERR_STS_INVALID_PARAM  Invalid parameter.
 * @return RET_ERR_STS_INVALID_LENGTH Invalid frame length.
 * @return RET_ERR_STS_CRC_ERROR      CRC validation failed.
 */
static inline ret_err_sts_t
rc_frame_decode(const uint8_t* rawBuf, uint16_t rawLen, rc_frame_t* frame)
{
    uint16_t payloadLen;
    uint16_t expectedLen;

    if((rawBuf == NULL) || (frame == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(rawLen < RC_FRAME_MIN_LEN)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    /* Payload length */
    payloadLen = rc_read_u16(&rawBuf[5U]);

    if(payloadLen > RC_MAX_PAYLOAD_LEN)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    /* Expected complete frame length */
    expectedLen = (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen + RC_FRAME_CRC_LEN);

    if(rawLen != expectedLen)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    /* CRC validation */
    if(!rc_validate_crc(rawBuf, rawLen))
    {
        return RET_ERR_STS_CRC_ERROR;
    }

    /* Decode header */
    frame->version = rc_get_version(rawBuf[0U]);
    frame->type    = rc_get_type(rawBuf[0U]);
    frame->flags   = rc_get_flags(rawBuf[0U]);

    /* Decode message information */
    frame->messageId  = (uint8_t)rawBuf[1U];
    frame->transport  = (uint8_t)rawBuf[2U];
    frame->sequence   = rc_read_u16(&rawBuf[3U]);
    frame->payloadLen = payloadLen;

    /* Payload points into received buffer */
    if(payloadLen > 0U)
    {
        frame->payload = &rawBuf[RC_FRAME_FIXED_LEN];
    }
    else
    {
        frame->payload = NULL;
    }

    return RET_ERR_STS_OK;
}


/* =========================================================================
 * FRAME LENGTH
 * ========================================================================= */

/**
 * @brief Calculate complete frame length.
 *
 * @param[in] payloadLen Application payload length.
 *
 * @return Complete frame length.
 * @return 0 if payload length is invalid.
 */
static inline uint16_t rc_get_frame_length(uint16_t payloadLen)
{
    if(payloadLen > RC_MAX_PAYLOAD_LEN)
    {
        return 0U;
    }

    return (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen + RC_FRAME_CRC_LEN);
}


#endif /* RC_PROTOCOL_H */