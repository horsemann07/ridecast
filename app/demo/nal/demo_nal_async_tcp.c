#include "demo_nal_tcp_tls.h"

// Public TCP echo server for testing
#define DEMO_NAL_ASYNC_TCP_SERVER_IP   "tcpbin.com"
#define DEMO_NAL_ASYNC_TCP_SERVER_PORT 4242
#define DEMO_NAL_ASYNC_TCP_MESSAGE     "Hello from Async TCP demo!"

/* ------------------------------------------------------------------------- */
/* ============================================================
 * Simple event callback for async client
 * ============================================================ */
static void app_event_handler(nalEvent_t event, void* data, size_t len, void* user_ctx)
{
    nalHandle_t* handle = (nalHandle_t*)user_ctx;

    switch(event)
    {
    case NAL_EVENT_CONNECTED:
        DEMO_LOGI("Connected to server!");
        nalNetworkSendAsync(handle, DEMO_NAL_ASYNC_TCP_MESSAGE,
                            strlen(DEMO_NAL_ASYNC_TCP_MESSAGE));
        break;

    case NAL_EVENT_DATA_RECEIVED:
        DEMO_LOGI("Data received (%u bytes): %.*s", (unsigned)len, (int)len, (char*)data);
        break;

    case NAL_EVENT_DISCONNECTED:
        DEMO_LOGW("Disconnected from server.");
        break;

    case NAL_EVENT_ERROR:
        DEMO_LOGE("Connection error occurred!");
        break;

    default:
        break;
    }
}

/* ------------------------------------------------------------------------- */

int main(void)
{
    nalHandle_t handle;

    if(nalNetworkInit(&handle) != ERR_STS_OK)
    {
        DEMO_LOGE("Failed to initialize handle.");
        return -1;
    }

    DEMO_LOGI("Connecting to echo server (TCP %s:%d)...",
              DEMO_NAL_ASYNC_TCP_SERVER_IP, DEMO_NAL_ASYNC_TCP_SERVER_PORT);

    errStatus_t sts =
    nalNetworkConnectAsync(&handle, DEMO_NAL_ASYNC_TCP_SERVER_IP, DEMO_NAL_ASYNC_TCP_SERVER_PORT,
                           NAL_SCHEME_PLAIN, app_event_handler, &handle);
    if(sts != ERR_STS_OK)
    {
        DEMO_LOGE("Connection start failed (status=%d)", sts);
        return -1;
    }

    /* Application main loop */
    while(1)
    {
        osDelay(1000); // Let async worker handle events
    }

    return 0;
}

/* ------------------------------------------------------------------------- */