/**
 * @brief USB virtual serial terminal
 */

#include "usb_datachannel.h"
#include "usb_uvc.h"

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

    usb_enable(BOARD_USB_DISC_DEV, (uint8_t)BOARD_USB_DISC_BIT);
}

