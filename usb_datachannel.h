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
//
//    // Roger Clark. Added dummy function so that existing Arduino sketches which specify baud rate will compile.
//    void begin(unsigned long);
//    void begin(unsigned long, uint8_t);
//    void end(void);
//
//    virtual int available(void);// Changed to virtual
//
//    uint32 read(uint8 * buf, uint32 len);
//    // uint8  read(void);
//
//    // Roger Clark. added functions to support Arduino 1.0 API
//    virtual int peek(void);
//    virtual int read(void);
//    int availableForWrite(void);
//    virtual void flush(void);
//
//
//    size_t write(uint8);
//    size_t write(const char *str);
//    size_t write(const uint8*, uint32);
//
//    uint8 getRTS();
//    uint8 getDTR();
//    uint8 pending();
//
//    /* SukkoPera: This is the Arduino way to check if an USB CDC serial
//     * connection is open.
//
//     * Used for instance in cardinfo.ino.
//     */
//    operator bool();
//
//    /* Old libmaple way to check for serial connection.
//     *
//     * Deprecated, use the above.
//     */
//    uint8 isConnected() __attribute__((deprecated("Use !Serial instead"))) { return (bool) *this; }

protected:
    static bool _hasBegun;
};

#endif
