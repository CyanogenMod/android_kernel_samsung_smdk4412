#if !defined(_QCSERIAL_H_)
#define _QCSERIAL_H_
#include <linux/wakelock.h>

#define PRODUCT_DLOAD "QHSUSB_DLOAD"
#define PRODUCT_SAHARA "QHSUSB__BULK"

extern void mdm_set_chip_configuration(bool dload);

#endif
