#ifndef _USB_DATACHANNEL_H_
#define _USB_DATACHANNEL_H_

#include "boards.h"

/**
 * @brief Virtual serial terminal.
 */
class USBDataChannel {
public:
    USBDataChannel(void);

    void begin(void);

protected:
    static bool _hasBegun;
};

#endif
