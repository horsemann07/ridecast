

#include "rc_protocol.h"

#include <string.h>


/* =========================================================================
 * Private Constants
 * ========================================================================= */

/*
 * Frame layout:
 *
 * Byte 0     : Header
 * Byte 1     : Message ID
 * Byte 2     : Transport
 * Byte 3-4   : Sequence
 * Byte 5-6   : Payload Length
 * Byte 7..N  : Payload
 * Byte N+1-2 : CRC16
 */

#define RC_FRAME_HEADER_OFFSET    0U
#define RC_FRAME_MSG_ID_OFFSET    1U
#define RC_FRAME_TRANSPORT_OFFSET 2U
#define RC_FRAME_SEQUENCE_OFFSET  3U
#define RC_FRAME_LENGTH_OFFSET    5U
#define RC_FRAME_PAYLOAD_OFFSET   7U

#define RC_FRAME_FIXED_LEN        7U
#define RC_FRAME_CRC_LEN          2U
#define RC_FRAME_MIN_LEN          (RC_FRAME_FIXED_LEN + RC_FRAME_CRC_LEN)

#define RC_CRC_POLYNOMIAL         0x1021U
#define RC_CRC_INITIAL_VALUE      0xFFFFU


/* =========================================================================
 * Private Functions
 * ========================================================================= */

/**
 * @brief Calculate CRC16-CCITT.
 *
 * CRC is calculated over:
 *
 *     HEADER + MSG_ID + TRANSPORT + SEQUENCE + LENGTH + PAYLOAD
 *
 * CRC bytes themselves are not included in the calculation.
 *
 * @param[in] data Data buffer.
 * @param[in] len  Number of bytes.
 *
 * @return Calculated CRC16.
 */
static uint16_t rc_protocol_calculate_crc(const uint8_t* data, uint16_t len)
{
    uint16_t crc = RC_CRC_INITIAL_VALUE;

    if((data == NULL) && (len > 0U))
    {
        return 0U;
    }

    for(uint16_t i = 0U; i < len; i++)
    {
        crc ^= ((uint16_t)data[i] << 8U);

        for(uint8_t bit = 0U; bit < 8U; bit++)
        {
            if((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((crc << 1U) ^ RC_CRC_POLYNOMIAL);
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
 * @brief Write a 16-bit value in big-endian format.
 */
static void rc_protocol_write_u16(uint8_t* buffer, uint16_t value)
{
    buffer[0] = (uint8_t)((value >> 8U) & 0xFFU);
    buffer[1] = (uint8_t)(value & 0xFFU);
}


/**
 * @brief Read a 16-bit value in big-endian format.
 */
static uint16_t rc_protocol_read_u16(const uint8_t* buffer)
{
    return (uint16_t)(((uint16_t)buffer[0] << 8U) | ((uint16_t)buffer[1]));
}


/* =========================================================================
 * Header Helpers
 * ========================================================================= */

/**
 * @brief Create RideCast protocol header.
 *
 * Header format:
 *
 *     Bits 7-6 : Version
 *     Bits 5-4 : Message Type
 *     Bits 3-0 : Flags
 */
uint8_t rc_make_header(uint8_t version, rc_type_t type, uint8_t flags)
{
    uint8_t header = 0U;

    header |= (uint8_t)((version & 0x03U) << 6U);
    header |= (uint8_t)((type & 0x03U) << 4U);
    header |= (uint8_t)(flags & 0x0FU);

    return header;
}


/**
 * @brief Get protocol version from header.
 */
uint8_t rc_get_version(uint8_t header)
{
    return (uint8_t)((header >> 6U) & 0x03U);
}


/**
 * @brief Get message type from header.
 */
rc_type_t rc_get_type(uint8_t header)
{
    return (rc_type_t)((header >> 4U) & 0x03U);
}


/**
 * @brief Get flags from header.
 */
uint8_t rc_get_flags(uint8_t header)
{
    return (uint8_t)(header & 0x0FU);
}


/* =========================================================================
 * CRC
 * ========================================================================= */

/**
 * @brief Calculate CRC for a complete frame without CRC bytes.
 *
 * @param[in] frame Frame data excluding CRC.
 * @param[in] length Frame length excluding CRC.
 *
 * @return CRC16 value.
 */
uint16_t rc_protocol_crc16(const uint8_t* frame, uint16_t length)
{
    return rc_protocol_calculate_crc(frame, length);
}


/**
 * @brief Validate CRC of a complete frame.
 *
 * @param[in] frame      Complete frame including CRC.
 * @param[in] frameLen   Complete frame length.
 *
 * @return true if CRC is valid.
 * @return false if CRC is invalid.
 */
bool rc_protocol_validate_crc(const uint8_t* frame, uint16_t frameLen)
{
    uint16_t calculatedCrc;
    uint16_t receivedCrc;

    if((frame == NULL) || (frameLen < RC_FRAME_MIN_LEN))
    {
        return false;
    }

    calculatedCrc =
    rc_protocol_calculate_crc(frame, (uint16_t)(frameLen - RC_FRAME_CRC_LEN));

    receivedCrc = rc_protocol_read_u16(&frame[frameLen - RC_FRAME_CRC_LEN]);

    return (calculatedCrc == receivedCrc);
}


/* =========================================================================
 * Frame Encoding
 * ========================================================================= */

/**
 * @brief Encode a control/status/ACK frame.
 *
 * Creates a frame without payload.
 *
 * Frame:
 *
 *     HEADER | MSG_ID | TRANSPORT | SEQ | LENGTH | CRC16
 *
 * @param[out] buffer    Output frame buffer.
 * @param[in]  bufferLen Output buffer size.
 * @param[in]  type      Message type.
 * @param[in]  messageId Message ID.
 * @param[in]  transport Wireless transport.
 * @param[in]  sequence  Sequence number.
 *
 * @return Encoded frame length.
 * @return 0 on error.
 */
uint16_t rc_encode_simple(uint8_t* buffer,
                          uint16_t bufferLen,
                          rc_type_t type,
                          rc_msg_id_t messageId,
                          rc_transport_t transport,
                          uint16_t sequence)
{
    uint16_t crc;
    uint16_t frameLen = RC_FRAME_FIXED_LEN + RC_FRAME_CRC_LEN;

    if((buffer == NULL) || (bufferLen < frameLen))
    {
        return 0U;
    }

    buffer[RC_FRAME_HEADER_OFFSET] = rc_make_header(RC_PROTOCOL_VERSION, type, 0U);

    buffer[RC_FRAME_MSG_ID_OFFSET] = (uint8_t)messageId;

    buffer[RC_FRAME_TRANSPORT_OFFSET] = (uint8_t)transport;

    rc_protocol_write_u16(&buffer[RC_FRAME_SEQUENCE_OFFSET], sequence);

    rc_protocol_write_u16(&buffer[RC_FRAME_LENGTH_OFFSET], 0U);

    crc = rc_protocol_calculate_crc(buffer, RC_FRAME_FIXED_LEN);

    rc_protocol_write_u16(&buffer[RC_FRAME_FIXED_LEN], crc);

    return frameLen;
}


/**
 * @brief Encode a data frame.
 *
 * Creates:
 *
 *     HEADER | SEND_DATA | TRANSPORT | SEQ |
 *     LENGTH | PAYLOAD | CRC16
 *
 * Payload is treated as opaque data.
 *
 * @param[out] buffer     Output frame buffer.
 * @param[in]  bufferLen  Output buffer size.
 * @param[in]  transport  Destination/source transport.
 * @param[in]  sequence   Sequence number.
 * @param[in]  payload    Application payload.
 * @param[in]  payloadLen Payload length.
 *
 * @return Encoded frame length.
 * @return 0 on error.
 */
uint16_t rc_encode_data(uint8_t* buffer,
                        uint16_t bufferLen,
                        rc_transport_t transport,
                        uint16_t sequence,
                        const uint8_t* payload,
                        uint16_t payloadLen)
{
    uint16_t crc;
    uint16_t frameLen;

    if((buffer == NULL) || (payload == NULL) || (payloadLen == 0U) ||
       (payloadLen > RC_MAX_PAYLOAD_LEN))
    {
        return 0U;
    }

    frameLen = (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen + RC_FRAME_CRC_LEN);

    if(bufferLen < frameLen)
    {
        return 0U;
    }

    buffer[RC_FRAME_HEADER_OFFSET] = rc_make_header(RC_PROTOCOL_VERSION, eRcTypeData, 0U);
    buffer[RC_FRAME_MSG_ID_OFFSET] = (uint8_t)eRcMsgSendData;
    buffer[RC_FRAME_TRANSPORT_OFFSET] = (uint8_t)transport;

    rc_protocol_write_u16(&buffer[RC_FRAME_SEQUENCE_OFFSET], sequence);

    rc_protocol_write_u16(&buffer[RC_FRAME_LENGTH_OFFSET], payloadLen);

    memcpy(&buffer[RC_FRAME_PAYLOAD_OFFSET], payload, payloadLen);

    crc = rc_protocol_calculate_crc(buffer, (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen));

    rc_protocol_write_u16(&buffer[RC_FRAME_FIXED_LEN + payloadLen], crc);

    return frameLen;
}


/* =========================================================================
 * Frame Decoding
 * ========================================================================= */

/**
 * @brief Decode and validate a RideCast frame.
 *
 * The payload pointer returned in rc_frame_t points directly into
 * the input buffer. No payload memory is allocated or copied.
 *
 * @param[in]  buffer    Received frame buffer.
 * @param[in]  bufferLen Number of bytes received.
 * @param[out] frame     Decoded frame information.
 *
 * @return RET_ERR_STS_OK on success.
 * @return RET_ERR_STS_INVALID_PARAM for invalid parameters.
 * @return RET_ERR_STS_INVALID_LENGTH for invalid frame length.
 * @return RET_ERR_STS_CRC_ERROR for CRC failure.
 */
ret_err_sts_t rc_frame_decode(const uint8_t* buffer, uint16_t bufferLen, rc_frame_t* frame)
{
    uint16_t payloadLen;
    uint16_t expectedLen;
    uint8_t header;

    if((buffer == NULL) || (frame == NULL))
    {
        return RET_ERR_STS_INVALID_PARAM;
    }

    if(bufferLen < RC_FRAME_MIN_LEN)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    header = buffer[RC_FRAME_HEADER_OFFSET];

    payloadLen = rc_protocol_read_u16(&buffer[RC_FRAME_LENGTH_OFFSET]);

    if(payloadLen > RC_MAX_PAYLOAD_LEN)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    expectedLen = (uint16_t)(RC_FRAME_FIXED_LEN + payloadLen + RC_FRAME_CRC_LEN);

    if(bufferLen != expectedLen)
    {
        return RET_ERR_STS_INVALID_LENGTH;
    }

    if(!rc_protocol_validate_crc(buffer, bufferLen))
    {
        return RET_ERR_STS_CRC_ERROR;
    }

    frame->version = rc_get_version(header);
    frame->type = rc_get_type(header);
    frame->flags = rc_get_flags(header);
    frame->messageId = (rc_msg_id_t)buffer[RC_FRAME_MSG_ID_OFFSET];
    frame->transport = (rc_transport_t)buffer[RC_FRAME_TRANSPORT_OFFSET];
    frame->sequence = rc_protocol_read_u16(&buffer[RC_FRAME_SEQUENCE_OFFSET]);
    frame->payloadLen = payloadLen;
    if(payloadLen > 0U)
    {
        frame->payload = &buffer[RC_FRAME_PAYLOAD_OFFSET];
    }
    else
    {
        frame->payload = NULL;
    }

    return RET_ERR_STS_OK;
}