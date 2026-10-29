#include "nimble/nimble_osi_filesystem.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

int conf_save_one(const char *Key_type, void *buf, int len)
{
    FILE *containerFileHandle;
    int rlen = 0;

    containerFileHandle = osi_fopen(Key_type, "rb");
    rlen = osi_fwrite(buf, len, 0, containerFileHandle);

    if (rlen == 0)
    {
        return OSI_FILESYS_FAILURE;
    }

    return 0;
}

int conf_read_one(const char *Key_type, void *buf, int len)
{
    FILE *containerFileHandle;
    int rlen = 0;

    containerFileHandle = osi_fopen(Key_type, "rb");
    rlen = osi_fread(buf, len, 0, containerFileHandle);

    if (rlen == 0)
    {
        return OSI_FILESYS_FAILURE;
    }

    return 0;
}

#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */
