/**
 * @file    rc_protocol.h
 * @brief   RideCast <-> Host Communication Protocol
 *
 * FRAME FORMAT:
 * ┌─────────┬───────────────────┬─────────────────────────┐
 * │ Byte 0  │ Byte 1-2 (opt)    │ Byte 3+ (opt)           │
 * │ Header  │ Length (DATA only)│ Payload (DATA only)     │
 * └─────────┴───────────────────┴─────────────────────────┘
 *
 * Byte 0 (Header):
 * ┌─────────────┬─────────────┬──────────────────────┐
 * │  bits [1:0] │  bits [3:2] │  bits [7:4]          │
 * │    INTF     │    TYPE     │    SUBCODE            │
 * └─────────────┴─────────────┴──────────────────────┘
 *
 * INTF (bits 1:0)
 *   00 = WiFi
 *   01 = BLE
 *   10 = Bluetooth Classic
 *   11 = Reserved
 *
 * TYPE (bits 3:2)
 *   00 = CMD    - Host->RideCast command
 *   01 = STATUS - RideCast->Host event/status
 *   10 = DATA   - Bidirectional, Length(2) + Payload follow
 *   11 = ACK    - Acknowledgement
 *
 * SUBCODE (bits 7:4)
 *   CMD    : START, STOP, RECONNECT
 *   STATUS : CONNECTED, DISCONNECTED, AUTH_OK, AUTH_FAIL, FAILED, SLEEPING,
 * READY DATA   : RAW, NAV, NOTIF ACK    : OK, FAIL
 *
 * FRAME SIZES:
 *   CMD / STATUS / ACK  : 1 byte  (header only)
 *   CMD with params     : 1 byte header + 2 byte length + N byte params
 *   DATA                : 1 byte header + 2 byte length + N byte payload
 *                         max payload = 4096 bytes
 */

#ifndef RC_PROTOCOL_H
#define RC_PROTOCOL_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>
#include <bsp_err_sts.h>

/* =========================================================================
 * LIMITS
 * ========================================================================= */
#define RC_MAX_PAYLOAD_LEN    (4096U)
#define RC_FRAME_HEADER_LEN   (1U)
#define RC_FRAME_LEN_FIELD    (2U)
#define RC_MIN_DATA_FRAME_LEN (RC_FRAME_HEADER_LEN + RC_FRAME_LEN_FIELD)

/* WiFi START param sizes */
#define RC_WIFI_SSID_LEN (32U)
#define RC_WIFI_PASS_LEN (64U)
#define RC_WIFI_START_PARAMS \
    (RC_WIFI_SSID_LEN + RC_WIFI_PASS_LEN) /* 96 bytes */

/* =========================================================================
 * HEADER BIT MASKS & SHIFTS
 * ========================================================================= */
#define RC_HDR_INTF_MASK  (0x03U) /* bits [1:0] */
#define RC_HDR_INTF_SHIFT (0U)
#define RC_HDR_TYPE_MASK  (0x0CU) /* bits [3:2] */
#define RC_HDR_TYPE_SHIFT (2U)
#define RC_HDR_SUB_MASK   (0xF0U) /* bits [7:4] */
#define RC_HDR_SUB_SHIFT  (4U)

    /* =========================================================================
     * INTF - Interface selector (bits 1:0)
     * ========================================================================= */
    typedef enum
    {
        eRcIntfHeartBeat  = 0x00U, /**< Heartbeat            */
        eRcIntfWifi      = 0x01U, /**< WiFi                */
        eRcIntfBle       = 0x02U, /**< BLE                 */
        eRcIntfBtClassic = 0x03U, /**< Bluetooth Classic   */
        eRcIntfReserved  = 0x04U  /**< Reserved            */
    } rc_intf_t;

    /* =========================================================================
     * TYPE - Message type (bits 3:2)
     * ========================================================================= */
    typedef enum
    {
        eRcTypeCmd    = 0x00U, /**< Host -> RideCast command          */
        eRcTypeStatus = 0x01U, /**< RideCast -> Host status/event     */
        eRcTypeData   = 0x02U, /**< Bidirectional data                */
        eRcTypeAck    = 0x03U  /**< Acknowledgement                   */
    } rc_type_t;

    /* =========================================================================
     * SUBCODES - per TYPE (bits 7:4)
     * ========================================================================= */

    /** CMD subcodes (TYPE = eRcTypeCmd) */
    typedef enum
    {
        eRcCmdSubStart     = 0x00U, /**< Start interface (SSID+PASS for WiFi)   */
        eRcCmdSubStop      = 0x01U, /**< Stop interface                         */
        eRcCmdSubReconnect = 0x02U  /**< Force reconnect                        */
    } rc_cmd_sub_t;

    /** STATUS subcodes (TYPE = eRcTypeStatus) */
    typedef enum
    {
        eRCStsSubOk           = 0x00U, /**< Generic OK status                      */
        eRCStsSubFail         = 0x01U, /**< Generic FAIL status                    */
        eRcStsSubConnected    = 0x02U, /**< Interface connected                  */
        eRcStsSubDisconnected = 0x03U, /**< Interface disconnected               */
        eRcStsSubAuthOk       = 0x04U, /**< Authentication passed                */
        eRcStsSubAuthFail     = 0x05U, /**< Authentication failed                */
        eRcStsSubFailed       = 0x06U, /**< Max retries hit                      */
        eRcStsSubSleeping     = 0x07U, /**< Sleeping, waiting for Host cmd       */
        eRcStsSubReady        = 0x08U, /**< All up, data flow active             */
        eRcStsSubRetrying     = 0x09U  /**< Reconnect attempt in progress        */
    } rc_sts_sub_t;

    /** DATA subcodes (TYPE = eRcTypeData) */
    typedef enum
    {
        eRcDataSubRaw   = 0x00U, /**< Raw bytes                               */
        eRcDataSubNav   = 0x01U, /**< Navigation data from phone              */
        eRcDataSubNotif = 0x02U  /**< Notification data from phone            */
    } rc_data_sub_t;

    /** ACK subcodes (TYPE = eRcTypeAck) */
    typedef enum
    {
        eRcAckSubOk   = 0x00U, /**< Acknowledged OK                         */
        eRcAckSubFail = 0x01U  /**< Acknowledged FAIL                       */
    } rc_ack_sub_t;

    /* =========================================================================
     * DECODED FRAME - output of rc_frame_decode()
     * ========================================================================= */
    typedef struct
    {
        rc_intf_t intf;         /**< Which interface                    */
        rc_type_t type;         /**< CMD / STATUS / DATA / ACK          */
        uint8_t subcode;        /**< Subcode (cmd/sts/data/ack sub)     */
        uint16_t payloadLen;    /**< 0 if not DATA, else payload length */
        const uint8_t* payload; /**< Points into raw buffer, not copied */
    } rc_frame_t;

    /* =========================================================================
     * ENCODE / DECODE API
     * ========================================================================= */

    /**
     * @brief  Build a header byte from parts
     */
    static inline uint8_t rc_make_header(rc_intf_t intf, rc_type_t type, uint8_t subcode)
    {
        return (uint8_t)(((uint8_t)intf << RC_HDR_INTF_SHIFT) |
                         ((uint8_t)type << RC_HDR_TYPE_SHIFT) |
                         ((uint8_t)subcode << RC_HDR_SUB_SHIFT));
    }

    /**
     * @brief  Encode CMD or STATUS frame (no payload) into buf.
     * @return Number of bytes written (always 1)
     */
    static inline uint8_t
    rc_encode_simple(uint8_t* buf, rc_intf_t intf, rc_type_t type, uint8_t subcode)
    {
        buf[0U] = rc_make_header(intf, type, subcode);
        return 1U;
    }

    /**
     * @brief  Encode a DATA frame (header + 2-byte length + payload) into buf.
     * @param  buf         Output buffer (must be >= 3 + payloadLen bytes)
     * @param  intf        Interface
     * @param  subcode     Data subcode (rc_data_sub_t)
     * @param  payload     Data bytes
     * @param  payloadLen  Byte count (max RC_MAX_PAYLOAD_LEN)
     * @return Total bytes written, 0 on error
     */
    static inline uint16_t
    rc_encode_data(uint8_t* buf, rc_intf_t intf, uint8_t subcode, const uint8_t* payload, uint16_t payloadLen)
    {
        uint16_t total;

        if((buf == NULL) || (payload == NULL) || (payloadLen == 0U) ||
           (payloadLen > RC_MAX_PAYLOAD_LEN))
        {
            return 0U;
        }

        total = (uint16_t)(RC_FRAME_HEADER_LEN + RC_FRAME_LEN_FIELD + payloadLen);

        buf[0U] = rc_make_header(intf, eRcTypeData, subcode);
        buf[1U] = (uint8_t)((payloadLen >> 8U) & 0xFFU); /* Length high byte */
        buf[2U] = (uint8_t)(payloadLen & 0xFFU);         /* Length low byte  */

        /* Copy payload inline - no heap */
        {
            uint16_t i;
            for(i = 0U; i < payloadLen; i++)
            {
                buf[3U + i] = payload[i];
            }
        }

        return total;
    }
    
    /**
     * @brief  Decode a raw byte buffer into rc_frame_t.
     *         payload pointer points INTO rawBuf (zero copy).
     * @param  rawBuf   Received bytes
     * @param  rawLen   Byte count
     * @param  frame    Output decoded frame
     * @return true  = valid frame decoded
     * @return false = buffer too short or invalid
     */
    static inline bsp_err_sts_t rc_frame_decode(const uint8_t* rawBuf, uint16_t rawLen, rc_frame_t* frame)
    {
        uint16_t payLen;

        if((rawBuf == NULL) || (frame == NULL) || (rawLen < RC_FRAME_HEADER_LEN))
        {
            return BSP_ERR_STS_INVALID_PARAM;
        }

        frame->intf = (rc_intf_t)((rawBuf[0U] & RC_HDR_INTF_MASK) >> RC_HDR_INTF_SHIFT);
        frame->type = (rc_type_t)((rawBuf[0U] & RC_HDR_TYPE_MASK) >> RC_HDR_TYPE_SHIFT);
        frame->subcode = (uint8_t)((rawBuf[0U] & RC_HDR_SUB_MASK) >> RC_HDR_SUB_SHIFT);

        if(frame->type == eRcTypeData)
        {
            /* DATA frame needs at least header + 2 length bytes */
            if(rawLen < RC_MIN_DATA_FRAME_LEN)
            {
                return BSP_ERR_STS_INVALID_PARAM;
            }

            payLen = (uint16_t)(((uint16_t)rawBuf[1U] << 8U) | (uint16_t)rawBuf[2U]);

            if((payLen > RC_MAX_PAYLOAD_LEN) ||
               (rawLen < (uint16_t)(RC_MIN_DATA_FRAME_LEN + payLen)))
            {
                return BSP_ERR_STS_INVALID_PARAM;
            }

            frame->payloadLen = payLen;
            frame->payload    = &rawBuf[3U];
        }
        else
        {
            /* CMD / STATUS / ACK: no payload */
            frame->payloadLen = 0U;
            frame->payload    = NULL;
        }

        return BSP_ERR_STS_OK;
    }

#ifdef __cplusplus
}
#endif

#endif /* RC_PROTOCOL_H */