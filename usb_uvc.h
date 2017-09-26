/******************************************************************************
 * The MIT License
 *
 * Copyright (c) 2011 LeafLabs LLC.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 * 
 * modfied from libmaple/include/libmaple/usb_cdcacm.h
 *****************************************************************************/
 
 #ifndef _USB_UVC_H_
#define _USB_UVC_H_

#include <libmaple/libmaple_types.h>
#include <libmaple/gpio.h>
#include <libmaple/usb.h>

#include "uvc.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Descriptors, etc.
 */
#define USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION 11
//#define UVC_INTERFACE_ASSOCIATION_DESC_SIZE 8

typedef struct usb_descriptor_interface_association {
    uint8  bLength;               
    uint8  bDescriptorType;       
    uint8  bFirstInterface;       
    uint8  bInterfaceCount;       
    uint8  bFunctionClass;        
    uint8  bFunctionSubClass;     
    uint8  bFunctionProtocol;     
    uint8  iFunction;             
} __packed usb_descriptor_interface_association;

/*
 * Endpoint configuration
 */

#define USB_CTRL_ENDP            0
#define USB_CTRL_RX_ADDR         0x40
#define USB_CTRL_TX_ADDR         0x80
#define USB_CTRL_EPSIZE          0x40

#define USB_TX_ENDP              1
#define USB_TX_ADDR              0xC0
#define USB_TX_EPSIZE            0x40

#define USB_MANAGEMENT_ENDP      2
#define USB_MANAGEMENT_ADDR      0x100
#define USB_MANAGEMENT_EPSIZE    0x40

#define USB_RX_ENDP              3
#define USB_RX_ADDR              0x110
#define USB_RX_EPSIZE            0x40

#ifndef __cplusplus
#define USB_DECLARE_DEV_DESC(vid, pid)                          \
  {                                                             \
      .bLength            = sizeof(usb_descriptor_device),      \
      .bDescriptorType    = USB_DESCRIPTOR_TYPE_DEVICE,         \
      .bcdUSB             = 0x0200,                             \
      .bDeviceClass       = UVC_DEVICE_CLASS_MISCELLANEOUS,     \
      .bDeviceSubClass    = UVC_DEVICE_SUBCLASS,                \
      .bDeviceProtocol    = UVC_DEVICE_PROTOCOL,                \
      .bMaxPacketSize0    = 0x40,                               \
      .idVendor           = vid,                                \
      .idProduct          = pid,                                \
      .bcdDevice          = 0x0200,                             \
      .iManufacturer      = 0x01,                               \
      .iProduct           = 0x02,                               \
      .iSerialNumber      = 0x00,                               \
      .bNumConfigurations = 0x01,                               \
 }
#endif


/*
 * stm32f103 usb interface
 */

void usb_enable(gpio_dev*, uint8);
void usb_disable(gpio_dev*, uint8);


#ifdef __cplusplus
}
#endif

#endif
