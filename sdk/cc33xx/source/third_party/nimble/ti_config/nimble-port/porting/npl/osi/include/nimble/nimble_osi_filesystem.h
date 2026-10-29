#ifndef _NIMBLE_NPL_FILESYSTEM_H_
#define _NIMBLE_NPL_FILESYSTEM_H_

#include "syscfg/syscfg.h"

#if MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST)

#include <string.h>
#include "osi_kernel.h"

int conf_save_one(const char *Key_type, void *buf, int len);
int conf_read_one(const char *Key_type, void *buf, int len);

#endif /* MYNEWT_VAL(BLE_STORE_CONFIG_PERSIST) */

#endif  /* _NIMBLE_NPL_FILESYSTEM_H_ */
