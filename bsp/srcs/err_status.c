#include "err_status.h"

const char* err_status_err_to_name(errStatus_t status)
{
    switch(status)
    {
    // General & Common Errors
    case ERR_STS_OK:
        return "ERR_STS_OK";
    case ERR_STS_FAIL:
        return "ERR_STS_FAIL";
    case ERR_STS_UNKN:
        return "ERR_STS_UNKN";
    case ERR_STS_INVALID_PARAM:
        return "ERR_STS_INVALID_PARAM";
    case ERR_STS_INV_STATE:
        return "ERR_STS_INV_STATE";
    case ERR_STS_INV_SIZE:
        return "ERR_STS_INV_SIZE";
    case ERR_STS_INV_RESPONSE:
        return "ERR_STS_INV_RESPONSE";
    case ERR_STS_INV_CRC:
        return "ERR_STS_INV_CRC";
    case ERR_STS_INV_VERSION:
        return "ERR_STS_INV_VERSION";
    case ERR_STS_NOT_FOUND:
        return "ERR_STS_NOT_FOUND";
    case ERR_STS_NOT_EXIST:
        return "ERR_STS_NOT_EXIST";
    case ERR_STS_CFG_ERR:
        return "ERR_STS_CFG_ERR";
    case ERR_STS_INIT_FAIL:
        return "ERR_STS_INIT_FAIL";
    case ERR_STS_RD_FAIL:
        return "ERR_STS_RD_FAIL";
    case ERR_STS_WR_FAIL:
        return "ERR_STS_WR_FAIL";
    case ERR_STS_TIMEOUT:
        return "ERR_STS_TIMEOUT";
    case ERR_STS_BUSY:
        return "ERR_STS_BUSY";
    case ERR_STS_NO_MEM:
        return "ERR_STS_NO_MEM";
    case ERR_STS_NOT_RUNNING:
        return "ERR_STS_NOT_RUNNING";
    case ERR_STS_FUNC_NOT_SUPPORTED:
        return "ERR_STS_FUNC_NOT_SUPPORTED";
    case ERR_STS_NOT_INIT:
        return "ERR_STS_NOT_INIT";
    case ERR_STS_INTERNAL_ERROR:
        return "ERR_STS_INTERNAL_ERROR";

    // Network / Wireless / Transport Errors
    case ERR_STS_TRANSPORT_INIT_FAIL:
        return "ERR_STS_TRANSPORT_INIT_FAIL";
    case ERR_STS_CONNECT_FAILED:
        return "ERR_STS_CONNECT_FAILED";
    case ERR_STS_DISCONNECTED:
        return "ERR_STS_DISCONNECTED";
    case ERR_STS_SEND_FAILED:
        return "ERR_STS_SEND_FAILED";
    case ERR_STS_RECV_FAILED:
        return "ERR_STS_RECV_FAILED";
    case ERR_STS_AUTH_FAILED:
        return "ERR_STS_AUTH_FAILED";
    case ERR_STS_DNS_FAILED:
        return "ERR_STS_DNS_FAILED";
    case ERR_STS_INVALID_ADDR:
        return "ERR_STS_INVALID_ADDR";
    case ERR_STS_RESOURCE_BUSY:
        return "ERR_STS_RESOURCE_BUSY";
    case ERR_STS_BUFFER_OVERFLOW:
        return "ERR_STS_BUFFER_OVERFLOW";
    case ERR_STS_UNSUPPORTED_MODE:
        return "ERR_STS_UNSUPPORTED_MODE";
    case ERR_STS_COLLISION:
        return "ERR_STS_COLLISION";
    case ERR_STS_NO_ACK:
        return "ERR_STS_NO_ACK";
    case ERR_STS_PKT_TOO_LARGE:
        return "ERR_STS_PKT_TOO_LARGE";
    case ERR_STS_SECURITY_FAIL:
        return "ERR_STS_SECURITY_FAIL";
    case ERR_STS_HANDSHAKE_FAIL:
        return "ERR_STS_HANDSHAKE_FAIL";
    case ERR_STS_CERT_INVALID:
        return "ERR_STS_CERT_INVALID";
    case ERR_STS_SHUTDOWN_FAIL:
        return "ERR_STS_SHUTDOWN_FAIL";
    case ERR_STS_INVALID_SOCKET:
        return "ERR_STS_INVALID_SOCKET";
    case ERR_STS_UNKNOWN:
        return "ERR_STS_UNKNOWN";

    default:
        return "ERR_STS_UNKNOWN";
    }
}