/**
 * @brief USB virtual serial terminal
 */

#include "usb_datachannel.h"
#include "usb_uvc.h"
/*
 * Hooks used for bootloader reset signalling
 */


static void rxHook(unsigned, void*);
static void ifaceSetupHook(unsigned, void*);


/*
 * USBSerial interface
 */

#define USB_TIMEOUT 50
bool USBDataChannel::_hasBegun = false;


USBDataChannel::USBDataChannel(void) {

}

void USBDataChannel::begin(void) {
  

    if (_hasBegun)
        return;
    _hasBegun = true;

    usb_cdcacm_enable(BOARD_USB_DISC_DEV, (uint8_t)BOARD_USB_DISC_BIT);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_RX, rxHook);
    usb_cdcacm_set_hooks(USB_CDCACM_HOOK_IFACE_SETUP, ifaceSetupHook);

}

