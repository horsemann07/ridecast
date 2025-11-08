#include "demo_nal_tcp_tls.h"
#include <stddef.h>

// Public TLS echo server for testing

#define DEMO_NAL_SYNC_TLS_SERVER_HOST "example.com"
#define DEMO_NAL_SYNC_TLS_SERVER_PORT 443
#define DEMO_NAL_SYNC_TLS_TIMEOUT_MS  5000UL
#define DEMO_NAL_SYNC_TLS_REQUEST \
    "GET / HTTP/1.1\r\nHost: example.com\r\nConnection: close\r\n\r\n"

/* Example CA certificate (replace with real PEM) */
static const uint8_t ca_cert_pem[] = "-----BEGIN CERTIFICATE-----\n"
                                     "...YOUR CA CERTIFICATE...\n"
                                     "-----END CERTIFICATE-----\n";

static const size_t ca_cert_len = sizeof(ca_cert_pem) - 1;

/* ------------------------------------------------------------------------- */
int main(void)
{
    nalHandle_t handle;
    char recv_buf[1024];

    if(nalNetworkInit(&handle) != ERR_STS_OK)
    {
        DEMO_LOGE("Failed to init handle.");
        return -1;
    }

    // /* Load CA certificate */
    // nalSetCaCert(&handle, ca_cert_pem, ca_cert_len);

    DEMO_LOGI("Connecting securely to %s:%u...", DEMO_NAL_SYNC_TLS_SERVER_HOST,
              DEMO_NAL_SYNC_TLS_SERVER_PORT);

    if(nalNetworkConnect(&handle, DEMO_NAL_SYNC_TLS_SERVER_HOST,
                         DEMO_NAL_SYNC_TLS_SERVER_PORT, NAL_SCHEME_TLS) != ERR_STS_OK)
    {
        DEMO_LOGE("Secure connect failed.");
        return -1;
    }

    DEMO_LOGI("Connected. Sending request...");
    if(nalNetworkSendSync(&handle, DEMO_NAL_SYNC_TLS_REQUEST, strlen(DEMO_NAL_SYNC_TLS_REQUEST),
                          DEMO_NAL_SYNC_TLS_TIMEOUT_MS) <= 0)
    {
        DEMO_LOGE("Send failed.");
        nalNetworkDisconnectSync(&handle);
        return -1;
    }

    DEMO_LOGI("Waiting for response...");
    int n = nalNetworkRecvSync(&handle, recv_buf, sizeof(recv_buf) - 1,
                               DEMO_NAL_SYNC_TLS_TIMEOUT_MS);
    if(n > 0)
    {
        recv_buf[n] = '\0';
        DEMO_LOGI("Response:\n%s", recv_buf);
    }
    else
    {
        DEMO_LOGW("No data received or read error.");
    }

    nalNetworkDisconnectSync(&handle);
    DEMO_LOGI("Disconnected cleanly.");
    return 0;
}

/* ------------------------------------------------------------------------- */