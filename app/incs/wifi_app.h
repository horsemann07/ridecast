
/**
 * ============================================================================
 * @file    wifi_app.h
 * @brief   RideCast Wi-Fi application service interface.
 *
 * @details
 * Provides the application-level Wi-Fi Access Point and TCP service
 * controlled by the vehicle cluster (MASTER).
 *
 * RideCast acts as SLAVE. It must not enable the Wi-Fi radio until
 * the cluster explicitly requests Wi-Fi START.
 *
 * RESPONSIBILITIES:
 *   - Receive START and STOP requests from RideCast application logic.
 *   - Start the Wi-Fi AP using cluster-provided SSID/password.
 *   - Accept mobile application TCP connections.
 *   - Authenticate incoming TCP clients.
 *   - Send and receive mobile application data.
 *   - Report Wi-Fi and TCP state changes to RideCast.
 *   - Apply the configured bounded retry policy.
 *   - Return to an inactive state after STOP.
 *
 * ============================================================================
 * APPLICATION FLOW
 * ============================================================================
 *
 *                  VEHICLE CLUSTER (MASTER)
 *                            |
 *                            | UART START / STOP
 *                            v
 *                       uart_app
 *                            |
 *                            | Decoded protocol
 *                            v
 *                     ridecast_main
 *                            |
 *                            | wifi_app_start()
 *                            | wifi_app_stop()
 *                            | wifi_app_send()
 *                            v
 *                   +------------------+
 *                   |   Wi-Fi Queue    |
 *                   +---------+--------+
 *                             |
 *                             v
 *                   +------------------+
 *                   | Wi-Fi App Task   |
 *                   |------------------|
 *                   | State machine    |
 *                   | AP control       |
 *                   | TCP sockets      |
 *                   | Authentication   |
 *                   +---------+--------+
 *                             |
 *                             v
 *                    BSP Wi-Fi Driver
 *                             |
 *                             v
 *                      MOBILE PHONE
 *
 * ============================================================================
 * STATE MACHINE
 * ============================================================================
 *
 *     +-----------+
 *     |   IDLE    | <------------------------------+
 *     +-----+-----+                                |
 *           |                                      |
 *           | Cluster START                        |
 *           v                                      |
 *     +-----------+                                |
 *     |  WIFI_UP  |                                |
 *     +-----+-----+                                |
 *           | Wi-Fi AP ready                       |
 *           v                                      |
 *     +------------+                               |
 *     | TCP_LISTEN | <---------+                   |
 *     +-----+------+           |                   |
 *           |                  |                   |
 *           | TCP connected    | Connection retry  |
 *           v                  |                   |
 *     +------------+           |                   |
 *     |  TCP_AUTH  |-----------+                   |
 *     +-----+------+                               |
 *           |                                      |
 *           | Authentication successful            |
 *           v                                      |
 *     +-----------+                                |
 *     |   READY   | -------------------------------+
 *     +-----+-----+        Cluster STOP
 *           |
 *           | Connection lost / retry exhausted
 *           v
 *     +-----------+
 *     | SLEEPING  |
 *     +-----+-----+
 *           |
 *           | Cluster START
 *           +---------------------> WIFI_UP
 *
 * NOTE:
 *   - The diagram describes the intended lifecycle.
 *   - All active states must process STOP requests.
 *   - SLEEPING must not leave the Wi-Fi radio active.
 *
 * ============================================================================
 * RTOS OBJECT OWNERSHIP
 * ============================================================================
 *
 * app_main.c:
 *   - Creates application-owned queues and mutexes.
 *
 * rc_os_map.c:
 *   - Defines Wi-Fi queue and task configuration.
 *
 * wifi_app_init():
 *   - Obtains the centrally configured Wi-Fi queue.
 *   - Initializes the Wi-Fi application state.
 *   - Starts the Wi-Fi worker only when that worker is not already
 *     created by the central startup flow.
 *   - Does not start the Wi-Fi AP.
 *
 * wifi_app_deinit():
 *   - Stops the Wi-Fi service and releases TCP/Wi-Fi resources.
 *   - Does not delete centrally owned queues.
 *
 * ============================================================================
 * CONFIGURATION
 * ============================================================================
 *
 * All adjustable values are defined in app_config.h using the
 * configMODULE_SETTING naming convention.
 *
 * ============================================================================
 * WARNINGS
 * ============================================================================
 *
 * @warning
 *   Wi-Fi must remain disabled until the cluster sends START.
 *
 *   TCP connection establishment does not imply authentication.
 *   Application data must only be forwarded after authentication.
 *
 *   The existing token-based authentication is development-only.
 *   Token validation alone does not create an encrypted TCP channel.
 *   Production pairing and encrypted communication require separate
 *   implementation and validation.
 *
 *   A reconnect retry limit must be enforced. Once retries are
 *   exhausted, report failure and wait for the cluster decision.
 *
 *   A TCP receive callback may provide only part of an application
 *   message. TCP does not preserve application-message boundaries.
 *
 *   Callback data buffers must not be retained after the callback
 *   returns unless the receiver makes its own copy.
 *
 * ============================================================================
 * Change Log (newest first):
 * ============================================================================
 * 2026-10-25  Aligned configuration, RTOS ownership and state definitions.
 * 2026-05-23  Initial development.
 * ============================================================================
 */

#ifndef WIFI_APP_H
#define WIFI_APP_H

#include <stdint.h>
#include <stdbool.h>

#include "app_config.h"
#include "cmsis_os2.h"
#include "bsp_wifi.h"
#include "rc_protocol.h"
#include "bsp_err_sts.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* =========================================================================
     * CONFIGURATION VALIDATION
     * ========================================================================= */

#if (configWIFI_APP_SSID_MAX_LEN == 0U) || (configWIFI_APP_SSID_MAX_LEN > 32U)
    #error "Wi-Fi SSID capacity must be between 1 and 32 bytes"
#endif

#if (configWIFI_APP_PASS_MAX_LEN < 8U) || (configWIFI_APP_PASS_MAX_LEN > 64U)
    #error "Wi-Fi password capacity must be between 8 and 64 bytes"
#endif

#if (configWIFI_APP_TOKEN_MAX_LEN == 0U)
    #error "Wi-Fi authentication token buffer must be nonzero"
#endif

#if (configWIFI_RXTX_BUF_SIZE == 0U)
    #error "Wi-Fi communication buffer must be nonzero"
#endif

#if (configWIFI_RXTX_BUF_SIZE > UINT16_MAX)
    #error "Wi-Fi communication buffer exceeds 16-bit message length"
#endif

#if (configWIFI_APP_MAX_RETRY == 0U) || (configWIFI_APP_MAX_RETRY > UINT8_MAX)
    #error "Wi-Fi retry limit must be between 1 and 255"
#endif

#if (configWIFI_APP_RETRY_BASE_MS == 0U) || \
(configWIFI_APP_RETRY_BASE_MS > configWIFI_APP_RETRY_MAX_MS)
    #error "Invalid Wi-Fi retry backoff configuration"
#endif

#if (configWIFI_APP_RETRY_MAX_MS > UINT16_MAX)
    #error "Wi-Fi maximum retry delay exceeds uint16_t"
#endif

#if (configWIFI_APP_AUTH_TIMEOUT_MS == 0U)
    #error "Wi-Fi authentication timeout must be nonzero"
#endif


    /* =========================================================================
     * WI-FI APPLICATION STATUS CODES
     * ========================================================================= */

    /**
     * @brief Status events reported to the RideCast application.
     *
     * @note
     *   Values are kept unchanged to preserve existing callback behavior.
     *
     *   eWifiAppStsWifiReady means the Access Point is available.
     *   eWifiAppStsTcpConnected means a TCP socket was accepted.
     *   eWifiAppStsReady means authentication completed successfully.
     */
    typedef enum
    {
        eWifiAppStsWifiStarting     = 0x01U,
        eWifiAppStsWifiReady        = 0x02U,
        eWifiAppStsWifiConnected    = 0x03U,
        eWifiAppStsWifiDisconnected = 0x04U,
        eWifiAppStsWifiRetrying     = 0x05U,
        eWifiAppStsWifiFailed       = 0x06U,

        eWifiAppStsTcpListening    = 0x11U,
        eWifiAppStsTcpConnected    = 0x12U,
        eWifiAppStsTcpAuthOk       = 0x13U,
        eWifiAppStsTcpAuthFail     = 0x14U,
        eWifiAppStsTcpDisconnected = 0x15U,
        eWifiAppStsTcpRetrying     = 0x16U,
        eWifiAppStsTcpFailed       = 0x17U,

        eWifiAppStsReady    = 0x20U,
        eWifiAppStsSleeping = 0x21U,
        eWifiAppStsStopped  = 0x22U

    } wifi_app_sts_t;


    /* =========================================================================
     * APPLICATION CALLBACKS
     * ========================================================================= */

    /**
     * @brief Report a Wi-Fi application state change.
     *
     * @param[in] sts  Current event/status code.
     * @param[in] arg  Retry count, TCP port, or other event information.
     * @param[in] user Registered callback context.
     *
     * @note
     *   Called from the Wi-Fi worker task.
     *   Avoid long blocking operations in this callback.
     */
    typedef void (*wifi_app_sts_cb_t)(wifi_app_sts_t sts, uint32_t arg, void* user);


    /**
     * @brief Deliver authenticated phone data to RideCast.
     *
     * @param[in] data Received TCP bytes.
     * @param[in] len  Number of bytes received.
     * @param[in] user Registered callback context.
     *
     * @warning
     *   The callback receives a temporary buffer.
     *   Copy the bytes if they must remain available after return.
     */
    typedef void (*wifi_app_rx_cb_t)(const uint8_t* data, uint16_t len, void* user);


    /* =========================================================================
     * WI-FI START PARAMETERS
     * ========================================================================= */

    /**
     * @brief Credentials supplied by the vehicle cluster.
     *
     * @note
     *   The current UART START command uses fixed-width fields.
     *   SSID and password are not necessarily null-terminated.
     *
     *   The Wi-Fi implementation must enforce actual credential lengths
     *   and termination requirements before passing these fields to
     *   BSP APIs that expect C strings.
     */
    typedef struct
    {
        char ssid[configWIFI_APP_SSID_MAX_LEN];
        char pass[configWIFI_APP_PASS_MAX_LEN];

    } wifi_app_start_params_t;


    /* =========================================================================
     * WI-FI APPLICATION CONFIGURATION
     * ========================================================================= */

    /**
     * @brief Wi-Fi service configuration supplied during initialization.
     *
     * @warning
     *   authToken preserves the existing development authentication flow.
     *   It is not equivalent to secure pairing or encrypted transport.
     */
    typedef struct
    {
        char authToken[configWIFI_APP_TOKEN_MAX_LEN];

        uint16_t tcpPort;
        uint32_t authTimeoutMs;

        uint8_t maxRetry;
        uint16_t retryBaseMs;
        uint16_t retryMaxMs;

        wifi_app_sts_cb_t onStatus;
        wifi_app_rx_cb_t onRx;

        void* cbUser;

    } wifi_app_cfg_t;


    /* =========================================================================
     * INTERNAL COMMAND IDENTIFIERS
     * ========================================================================= */

    /**
     * @brief Commands posted to the existing Wi-Fi worker queue.
     *
     * @note
     *   These are internal application commands, not UART message IDs.
     */
    typedef enum
    {
        eWifiAppCmdStart = 1U,
        eWifiAppCmdStop  = 2U,
        eWifiAppCmdSend  = 3U

    } wifi_app_cmd_t;


    /* =========================================================================
     * INTERNAL STATE MACHINE
     * ========================================================================= */

    /**
     * @brief Internal Wi-Fi service states.
     */
    typedef enum
    {
        eWifiAppStateIdle      = 0U,
        eWifiAppStateWifiUp    = 1U,
        eWifiAppStateTcpListen = 2U,
        eWifiAppStateTcpAuth   = 3U,
        eWifiAppStateReady     = 4U,
        eWifiAppStateSleeping  = 5U

    } wifi_app_state_t;


    /* =========================================================================
     * WI-FI APPLICATION CONTEXT
     * ========================================================================= */

    /**
     * @brief Runtime context for the Wi-Fi application service.
     *
     * @note
     *   wifiQ refers to a centrally created RTOS queue.
     *
     *   Socket descriptors are initialized to -1, indicating that
     *   no TCP socket is currently open.
     */
    typedef struct
    {
        wifi_app_cfg_t cfg;
        wifi_app_start_params_t startParams;

        wifi_app_state_t state;

        osThreadId_t taskId;
        osMessageQueueId_t wifiQ;

        volatile uint8_t running;

        int32_t listenFd;
        int32_t clientFd;

        uint8_t wifiRetry;
        uint8_t tcpRetry;

        bspWifiHandle_t wifi;

    } wifi_app_t;


    /* =========================================================================
     * INTERNAL QUEUE MESSAGE
     * ========================================================================= */

    /**
     * @brief Command message passed to the Wi-Fi worker.
     *
     * @details
     * START:
     *   - cmd = eWifiAppCmdStart
     *   - startParams contains cluster-provided credentials
     *
     * STOP:
     *   - cmd = eWifiAppCmdStop
     *
     * SEND:
     *   - cmd = eWifiAppCmdSend
     *   - len and data contain bytes destined for the phone
     *
     * @note
     *   Stored by value in the CMSIS-RTOS2 message queue.
     */
    typedef struct
    {
        uint8_t cmd;
        uint16_t len;

        uint8_t data[configWIFI_RXTX_BUF_SIZE];

        wifi_app_start_params_t startParams;

    } wifi_app_msg_t;


    /* =========================================================================
     * PUBLIC API
     * ========================================================================= */

    /**
     * @brief Initialize the Wi-Fi application service.
     *
     * @details
     * Initializes the application context and attaches the configured
     * RTOS queue and worker task.
     *
     * Does not enable the Wi-Fi Access Point.
     *
     * @param[in,out] app Wi-Fi application context.
     * @param[in]     cfg Wi-Fi service configuration.
     *
     * @return RET_ERR_STS_OK on success or an error status.
     */
    ret_err_sts_t wifi_app_init(void);


    /**
     * @brief Stop and deinitialize the Wi-Fi application service.
     *
     * @param[in,out] app Wi-Fi application context.
     *
     * @return RET_ERR_STS_OK on success or an error status.
     *
     * @note
     *   The implementation must not delete centrally owned RTOS queues.
     */
    ret_err_sts_t wifi_app_deinit(void);


    /**
     * @brief Request Wi-Fi startup using cluster-provided credentials.
     *
     * @param[in,out] app    Wi-Fi application context.
     * @param[in]     params SSID and password supplied by the cluster.
     *
     * @return RET_ERR_STS_OK if accepted or an error status.
     *
     * @note
     *   A successful return indicates the START request was queued.
     *   It does not indicate that the Wi-Fi AP or TCP server is ready.
     */
    ret_err_sts_t wifi_app_start(const wifi_app_start_params_t* params);


    /**
     * @brief Request shutdown of the Wi-Fi AP and TCP service.
     *
     * @param[in,out] app Wi-Fi application context.
     *
     * @return RET_ERR_STS_OK if accepted or an error status.
     *
     * @note
     *   STOP must be processed regardless of the active Wi-Fi state.
     */
    ret_err_sts_t wifi_app_stop(void);


    /**
     * @brief Request transmission of data to the authenticated TCP client.
     *
     * @param[in,out] app  Wi-Fi application context.
     * @param[in]     data Bytes to transmit.
     * @param[in]     len  Number of bytes.
     *
     * @return RET_ERR_STS_OK if accepted or an error status.
     *
     * @warning
     *   The implementation must reject sending when authentication has
     *   not completed. It must also check for partial socket writes.
     */
    ret_err_sts_t wifi_app_send(const uint8_t* data, uint16_t len);


#ifdef __cplusplus
}
#endif

#endif
