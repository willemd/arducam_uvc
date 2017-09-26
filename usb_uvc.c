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
 * modified from libmaple/usb/stm32f1/usb_cdcacm.c
 *****************************************************************************/

#include <libmaple/usb.h>
#include <libmaple/nvic.h>


/* Private headers */
#include "usb_lib_globals.h"
#include "usb_reg_map.h"

/* usb_lib headers */
#include "usb_type.h"
#include "usb_core.h"
#include "usb_def.h"

#include "usb_uvc.h"
#include "usb_uvcvideo.h"

static void usbInit(void);
static void usbReset(void);
static RESULT usbDataSetup(uint8 request);
static RESULT usbNoDataSetup(uint8 request);
static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting);
static uint8* usbGetDeviceDescriptor(uint16 length);
static uint8* usbGetConfigDescriptor(uint16 length);
static uint8* usbGetStringDescriptor(uint16 length);
static void usbSetConfiguration(void);
static void usbSetDeviceAddress(void);

/*
 * Descriptors
 */

/* FIXME move to Wirish */
#define LEAFLABS_ID_VENDOR                0x1EAF
#define MAPLE_ID_PRODUCT                  0x0027
static const usb_descriptor_device usbDescriptor_Device =
    USB_DECLARE_DEV_DESC(LEAFLABS_ID_VENDOR, MAPLE_ID_PRODUCT);

typedef struct {
    usb_descriptor_config_header            Config_Header;
    usb_descriptor_interface_association    UVC_Interface_Association;  
    usb_descriptor_interface                UVC_Control_Interface;
    uvc_header_descriptor                   UVC_Interface_Header;
    uvc_camera_terminal_descriptor          UVC_Camera_Terminal;
    uvc_processing_unit_descriptor          UVC_Processing_Unit;
    uvc_extension_unit_descriptor           UVC_Extension_Unit;
    uvc_output_terminal_descriptor          UVC_Output_Unit;
    usb_descriptor_endpoint                 InterruptEndpoint;
    usb_descriptor_interface                UVC_Streaming_Interface;
    uvc_input_header_descriptor             UVC_VS_Interface_Header;
    uvc_format_uncompressed                 UVC_YUY2_format;
    uvc_frame_uncompressed                  UVC_YUY2_320_240_Frame;
    uvc_format_mjpeg                        UVC_MJPEG_Format;
    uvc_frame_mjpeg                         UVC_MJPEG_1600_1200_Frame;
    uvc_color_matching_descriptor           UVC_Color_Matching;
    usb_descriptor_endpoint                 DataInEndpoint;
} __packed usb_descriptor_config;

#define MAX_POWER (100 >> 1)

#define VS_HEADER_SIZ (unsigned int)(UVC_DT_INPUT_HEADER_SIZE(2, 1) +\
UVC_DT_FORMAT_UNCOMPRESSED_SIZE + \
UVC_DT_FRAME_UNCOMPRESSED_SIZE(1)  +  \
UVC_DT_FRAME_UNCOMPRESSED_SIZE(1)  +  \
UVC_DT_FRAME_UNCOMPRESSED_SIZE(1)  +  \
UVC_DT_FORMAT_MJPEG_SIZE +\
UVC_DT_FRAME_MJPEG_SIZE(1) +\
UVC_DT_FRAME_MJPEG_SIZE(1) +\
UVC_DT_FRAME_MJPEG_SIZE(1) +\
UVC_DT_COLOR_MATCHING_SIZE)


#define VC_TERMINAL_SIZ (unsigned int) ( UVC_DT_HEADER_SIZE(1) +\
UVC_DT_CAMERA_TERMINAL_SIZE(2) +\
UVC_DT_PROCESSING_UNIT_SIZE(3) +\
UVC_DT_EXTENSION_UNIT_SIZE(1, 3) +\
UVC_DT_OUTPUT_TERMINAL_SIZE)

#define USB_UVC_VCIF_NUM 0
#define USB_UVC_VSIF_NUM 1

static const usb_descriptor_config usbDescriptor_Config = {
  .Config_Header = {
    .bLength                    = sizeof(usb_descriptor_config_header),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_CONFIGURATION,
    .wTotalLength               = sizeof(usb_descriptor_config),
    .bNumInterfaces             = 0x02,
    .bConfigurationValue        = 0x01,
    .iConfiguration             = 0x00,
    .bmAttributes               = (USB_CONFIG_ATTR_BUSPOWERED | USB_CONFIG_ATTR_SELF_POWERED),
    .bMaxPower                  = MAX_POWER,
  },
  .UVC_Interface_Association = {
    .bLength                    = sizeof(usb_descriptor_interface_association),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_INTERFACE_ASSOCIATION,
    .bFirstInterface            = 0x00,
    .bInterfaceCount            = 0x02,
    .bFunctionClass             = CC_VIDEO,
    .bFunctionSubClass          = SC_VIDEO_INTERFACE_COLLECTION,
    .bFunctionProtocol          = PC_PROTOCOL_UNDEFINED,
    .iFunction                  = 0x01,
  },
  .UVC_Control_Interface = {
    .bLength                    = sizeof(usb_descriptor_interface),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_INTERFACE,
    .bInterfaceNumber           = USB_UVC_VCIF_NUM,
    .bAlternateSetting          = 0,
    .bNumEndpoints              = 1,
    .bInterfaceClass            = CC_VIDEO,
    .bInterfaceSubClass         = SC_VIDEOCONTROL,
    .bInterfaceProtocol         = PC_PROTOCOL_UNDEFINED,
    .iInterface                 = 1,
  },
  .UVC_Interface_Header = {
    .bLength                    = UVC_DT_HEADER_SIZE(1),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = UVC_VC_HEADER,
    .bcdUVC                     = UVC_VERSION,
    .wTotalLength               = VC_TERMINAL_SIZ,
    .dwClockFrequency           = 0x005B8D80,
    .bInCollection              = 1,
    .baInterfaceNr              = 1,
  },
  .UVC_Camera_Terminal = {
    .bLength                    = UVC_DT_CAMERA_TERMINAL_SIZE(2),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VC_INPUT_TERMINAL,
    .bTerminalID                = 2,
    .wTerminalType              = ITT_CAMERA,
    .bAssocTerminal             = 0,
    .iTerminal                  = 0,
    .wObjectiveFocalLengthMin   = 0x0000,
    .wObjectiveFocalLengthMax   = 0x0000,
    .wOcularFocalLength         = 0x0000,
    .bControlSize               = 2,
    .bmControls                 = {0x00, 0x00},
  },
  .UVC_Processing_Unit = {
    .bLength                    = UVC_DT_PROCESSING_UNIT_SIZE(3),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = UVC_VC_PROCESSING_UNIT,
    .bUnitID                    = 1,
    .bSourceID                  = 2,
    .wMaxMultiplier             = 0x400,
    .bControlSize               = 3,
    .bmControls                 = {0x00, 0x00, 0x00},
    .iProcessing                = 1,
    .bmVideoStandards           = 0x00,
  },
  .UVC_Extension_Unit = {
    .bLength                    = UVC_DT_EXTENSION_UNIT_SIZE(1, 3),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = UVC_VC_EXTENSION_UNIT,
    .bUnitID                    = 3,
    .guidExtensionCode          = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    .bNumControls               = 0,
    .bNrInPins                  = 1,
    .baSourceID                 = 1,
    .bControlSize               = 3,
    .bmControls                 = {0x00, 0x00, 0x00},
    .iExtension                 = 0,
  },
  .UVC_Output_Unit = {
    .bLength                    = UVC_DT_OUTPUT_TERMINAL_SIZE,
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = UVC_VC_OUTPUT_TERMINAL,
    .bTerminalID                = 4,
    .wTerminalType              = UVC_TT_STREAMING,
    .bAssocTerminal             = 0,
    .bSourceID                  = 1,
    .iTerminal                  = 1,
  },
  .InterruptEndpoint = {
    .bLength                    = sizeof(usb_descriptor_endpoint),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_ENDPOINT,
    .bEndpointAddress           = (USB_DESCRIPTOR_ENDPOINT_IN | USB_MANAGEMENT_ENDP),
    .bmAttributes               = USB_EP_TYPE_INTERRUPT,
    .wMaxPacketSize             = USB_MANAGEMENT_EPSIZE,
    .bInterval                  = 0xFF,
  },
  .UVC_Streaming_Interface = {
    .bLength                    = sizeof(usb_descriptor_interface),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_INTERFACE,
    .bInterfaceNumber           = USB_UVC_VSIF_NUM,
    .bAlternateSetting          = 0,
    .bNumEndpoints              = 1,
    .bInterfaceClass            = CC_VIDEO,
    .bInterfaceSubClass         = SC_VIDEOSTREAMING,
    .bInterfaceProtocol         = PC_PROTOCOL_UNDEFINED,
    .iInterface                 = 1,
  },
  .UVC_VS_Interface_Header = {
    .bLength                    = UVC_DT_INPUT_HEADER_SIZE(2, 1),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VS_INPUT_HEADER,
    .bNumFormats                = 2,
    .wTotalLength               = VS_HEADER_SIZ,
    .bEndpointAddress           = (USB_DESCRIPTOR_ENDPOINT_IN | USB_TX_ENDP),
    .bmInfo                     = 0x00,
    .bTerminalLink              = 4,
    .bStillCaptureMethod        = 0x00,
    .bTriggerSupport            = 0x00,
    .bTriggerUsage              = 0x00,
    .bControlSize               = 1,
    .bmaControls                = { 0x00, 0x00},
  },
  .UVC_YUY2_format = {
    .bLength                    = UVC_DT_FORMAT_UNCOMPRESSED_SIZE,
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VS_FORMAT_UNCOMPRESSED,
    .bFormatIndex               = 1,
    .bNumFrameDescriptors       = 1,
    .guidFormat                 = {0x59, 0x55, 0x59, 0x32, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00,0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71},
    .bBitsPerPixel              = 16,
    .bDefaultFrameIndex         = 1,
    .bAspectRatioX              = 0x00,
    .bAspectRatioY              = 0x00,
    .bmInterfaceFlags           = 0x00,
    .bCopyProtect               = 0x00,
  },
  .UVC_YUY2_320_240_Frame = {
    .bLength                    = UVC_DT_FRAME_UNCOMPRESSED_SIZE(1),
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VS_FRAME_UNCOMPRESSED,
    .bFrameIndex                = 1,
    .bmCapabilities             = 0,
    .wWidth                     = 320,
    .wHeight                    = 240,
    .dwMinBitRate               = 0x01194000,
    .dwMaxBitRate               = 0x01194000,
    .dwMaxVideoFrameBufferSize  = 0x500,
    .dwDefaultFrameInterval     = 2000000,
    .bFrameIntervalType         = 1,
    .dwFrameInterval            = 2000000,
  },
  .UVC_MJPEG_Format = {
    .bLength                    = UVC_DT_FORMAT_MJPEG_SIZE,
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VS_FORMAT_MJPEG,
    .bFormatIndex               = 2,
    .bNumFrameDescriptors       = 1,
    .bmFlags                    = 0x00,
    .bDefaultFrameIndex         = 1,
    .bAspectRatioX              = 0x00,
    .bAspectRatioY              = 0x00,
    .bmInterfaceFlags           = 0x00,
    .bCopyProtect               = 0x00,
  },
  .UVC_MJPEG_1600_1200_Frame = {
    .bLength                    = UVC_DT_FRAME_MJPEG_SIZE(1),  
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = VS_FRAME_MJPEG,
    .bFrameIndex                = 1,
    .bmCapabilities             = 0,
    .wWidth                     = 1600,
    .wHeight                    = 1200,
    .dwMinBitRate               = 4800000,
    .dwMaxBitRate               = 4800000,
    .dwMaxVideoFrameBufferSize  = 0x500,
    .dwDefaultFrameInterval     = 2000000,
    .bFrameIntervalType         = 1,
    .dwFrameInterval            = 2000000,
  },
  .UVC_Color_Matching = {
    .bLength                    = UVC_DT_COLOR_MATCHING_SIZE,
    .bDescriptorType            = CS_INTERFACE,
    .bDescriptorSubType         = UVC_VS_COLORFORMAT,
    .bColorPrimaries            = 1,
    .bTransferCharacteristics   = 1,
    .bMatrixCoefficients        = 4,
  },
  .DataInEndpoint = {
    .bLength                    = sizeof(usb_descriptor_endpoint),
    .bDescriptorType            = USB_DESCRIPTOR_TYPE_ENDPOINT,
    .bEndpointAddress           = (USB_DESCRIPTOR_ENDPOINT_IN | USB_TX_ENDP),
    .bmAttributes               = USB_EP_TYPE_BULK,
    .wMaxPacketSize             = USB_TX_EPSIZE,
    .bInterval                  = 0x00,
  },
};

/*
  String Descriptors:

  we may choose to specify any or none of the following string
  identifiers:

  iManufacturer:    LeafLabs
  iProduct:         Maple
  iSerialNumber:    NONE
  iConfiguration:   NONE
  iInterface(CCI):  NONE
  iInterface(DCI):  NONE

*/

/* Unicode language identifier: 0x0409 is US English */
/* FIXME move to Wirish */
static const usb_descriptor_string usbDescriptor_LangID = {
    .bLength         = USB_DESCRIPTOR_STRING_LEN(1),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString         = {0x09, 0x04},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbDescriptor_iManufacturer = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(8),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'L', 0, 'e', 0, 'a', 0, 'f', 0,
                'L', 0, 'a', 0, 'b', 0, 's', 0},
};

/* FIXME move to Wirish */
static const usb_descriptor_string usbDescriptor_iProduct = {
    .bLength = USB_DESCRIPTOR_STRING_LEN(5),
    .bDescriptorType = USB_DESCRIPTOR_TYPE_STRING,
    .bString = {'M', 0, 'a', 0, 'p', 0, 'l', 0, 'e', 0},
};

static ONE_DESCRIPTOR Device_Descriptor = {
    (uint8*)&usbDescriptor_Device,
    sizeof(usb_descriptor_device)
};

static ONE_DESCRIPTOR Config_Descriptor = {
    (uint8*)&usbDescriptor_Config,
    sizeof(usb_descriptor_config)
};

#define N_STRING_DESCRIPTORS 3
static ONE_DESCRIPTOR String_Descriptor[N_STRING_DESCRIPTORS] = {
    {(uint8*)&usbDescriptor_LangID,       USB_DESCRIPTOR_STRING_LEN(1)},
    {(uint8*)&usbDescriptor_iManufacturer,USB_DESCRIPTOR_STRING_LEN(8)},
    {(uint8*)&usbDescriptor_iProduct,     USB_DESCRIPTOR_STRING_LEN(5)}
};

/*
 * Etc.
 */

/*
 * Endpoint callbacks
 */

static void (*ep_int_in[7])(void) =
    {NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

static void (*ep_int_out[7])(void) =
    {NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process,
     NOP_Process};

/*
 * Globals required by usb_lib/
 *
 * Mark these weak so they can be overriden to implement other USB
 * functionality.
 */

#define NUM_ENDPTS                0x04
__weak DEVICE Device_Table = {
    .Total_Endpoint      = NUM_ENDPTS,
    .Total_Configuration = 1
};

#define MAX_PACKET_SIZE            0x40  /* 64B, maximum for USB FS Devices */
__weak DEVICE_PROP Device_Property = {
    .Init                        = usbInit,
    .Reset                       = usbReset,
    .Process_Status_IN           = NOP_Process,
    .Process_Status_OUT          = NOP_Process,
    .Class_Data_Setup            = usbDataSetup,
    .Class_NoData_Setup          = usbNoDataSetup,
    .Class_Get_Interface_Setting = usbGetInterfaceSetting,
    .GetDeviceDescriptor         = usbGetDeviceDescriptor,
    .GetConfigDescriptor         = usbGetConfigDescriptor,
    .GetStringDescriptor         = usbGetStringDescriptor,
    .RxEP_buffer                 = NULL,
    .MaxPacketSize               = MAX_PACKET_SIZE
};

__weak USER_STANDARD_REQUESTS User_Standard_Requests = {
    .User_GetConfiguration   = NOP_Process,
    .User_SetConfiguration   = usbSetConfiguration,
    .User_GetInterface       = NOP_Process,
    .User_SetInterface       = NOP_Process,
    .User_GetStatus          = NOP_Process,
    .User_ClearFeature       = NOP_Process,
    .User_SetEndPointFeature = NOP_Process,
    .User_SetDeviceFeature   = NOP_Process,
    .User_SetDeviceAddress   = usbSetDeviceAddress
};


/*
 * stm32f103 usb interface
 */

void usb_enable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Present ourselves to the host. Writing 0 to "disc" pin must
     * pull USB_DP pin up while leaving USB_DM pulled down by the
     * transceiver. See USB 2.0 spec, section 7.1.7.3. */
   
  if (disc_dev!=NULL)
  {  
    gpio_set_mode(disc_dev, disc_bit, GPIO_OUTPUT_PP);
    gpio_write_bit(disc_dev, disc_bit, 0);
  }
  
    /* Initialize the USB peripheral. */
    usb_init_usblib(USBLIB, ep_int_in, ep_int_out);
}

void usb_disable(gpio_dev *disc_dev, uint8 disc_bit) {
    /* Turn off the interrupt and signal disconnect (see e.g. USB 2.0
     * spec, section 7.1.7.3). */
    nvic_irq_disable(NVIC_USB_LP_CAN_RX0);
  if (disc_dev!=NULL)
  {
    gpio_write_bit(disc_dev, disc_bit, 1);
  }
}

static void usbInit(void) {
    pInformation->Current_Configuration = 0;

    USB_BASE->CNTR = USB_CNTR_FRES;

    USBLIB->irq_mask = 0;
    USB_BASE->CNTR = USBLIB->irq_mask;
    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_CNTR_RESETM | USB_CNTR_SUSPM | USB_CNTR_WKUPM;
    USB_BASE->CNTR = USBLIB->irq_mask;

    USB_BASE->ISTR = 0;
    USBLIB->irq_mask = USB_ISR_MSK;
    USB_BASE->CNTR = USBLIB->irq_mask;

    nvic_irq_enable(NVIC_USB_LP_CAN_RX0);
    USBLIB->state = USB_UNCONNECTED;
}

#define BTABLE_ADDRESS        0x00
static void usbReset(void) {
    pInformation->Current_Configuration = 0;

    /* current feature is current bmAttributes */
    pInformation->Current_Feature = (USB_CONFIG_ATTR_BUSPOWERED |
                                     USB_CONFIG_ATTR_SELF_POWERED);

    USB_BASE->BTABLE = BTABLE_ADDRESS;

    /* setup control endpoint 0 */
    usb_set_ep_type(USB_EP0, USB_EP_EP_TYPE_CONTROL);
    usb_set_ep_tx_stat(USB_EP0, USB_EP_STAT_TX_STALL);
    usb_set_ep_rx_addr(USB_EP0, USB_CTRL_RX_ADDR);
    usb_set_ep_tx_addr(USB_EP0, USB_CTRL_TX_ADDR);
    usb_clear_status_out(USB_EP0);

    usb_set_ep_rx_count(USB_EP0, pProperty->MaxPacketSize);
    usb_set_ep_rx_stat(USB_EP0, USB_EP_STAT_RX_VALID);

    /* setup management endpoint 1  */
    usb_set_ep_type(USB_MANAGEMENT_ENDP, USB_EP_EP_TYPE_INTERRUPT);
    usb_set_ep_tx_addr(USB_MANAGEMENT_ENDP,
                       USB_MANAGEMENT_ADDR);
    usb_set_ep_tx_stat(USB_MANAGEMENT_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_MANAGEMENT_ENDP, USB_EP_STAT_RX_DISABLED);

    /* TODO figure out differences in style between RX/TX EP setup */

    /* set up data endpoint OUT (RX) */
    usb_set_ep_type(USB_RX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_rx_addr(USB_RX_ENDP, USB_RX_ADDR);
    usb_set_ep_rx_count(USB_RX_ENDP, USB_RX_EPSIZE);
    usb_set_ep_rx_stat(USB_RX_ENDP, USB_EP_STAT_RX_VALID);

    /* set up data endpoint IN (TX)  */
    usb_set_ep_type(USB_TX_ENDP, USB_EP_EP_TYPE_BULK);
    usb_set_ep_tx_addr(USB_TX_ENDP, USB_TX_ADDR);
    usb_set_ep_tx_stat(USB_TX_ENDP, USB_EP_STAT_TX_NAK);
    usb_set_ep_rx_stat(USB_TX_ENDP, USB_EP_STAT_RX_DISABLED);

    USBLIB->state = USB_ATTACHED;
    SetDeviceAddress(0);
}

static RESULT usbDataSetup(uint8 request) {
    uint8* (*CopyRoutine)(uint16) = 0;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
        switch (request) {
        default:
            break;
        }
    }

    if (CopyRoutine == NULL) {
        return USB_UNSUPPORT;
    }

    pInformation->Ctrl_Info.CopyData = CopyRoutine;
    pInformation->Ctrl_Info.Usb_wOffset = 0;
    (*CopyRoutine)(0);
    return USB_SUCCESS;
}

static RESULT usbNoDataSetup(uint8 request) {
    RESULT ret = USB_UNSUPPORT;

    if (Type_Recipient == (CLASS_REQUEST | INTERFACE_RECIPIENT)) {
        switch (request) {
            break;
        }

    }
    return ret;
}

static RESULT usbGetInterfaceSetting(uint8 interface, uint8 alt_setting) {
    if (alt_setting > 0) {
        return USB_UNSUPPORT;
    } else if (interface > 1) {
        return USB_UNSUPPORT;
    }

    return USB_SUCCESS;
}

static uint8* usbGetDeviceDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &Device_Descriptor);
}

static uint8* usbGetConfigDescriptor(uint16 length) {
    return Standard_GetDescriptorData(length, &Config_Descriptor);
}

static uint8* usbGetStringDescriptor(uint16 length) {
    uint8 wValue0 = pInformation->USBwValue0;

    if (wValue0 > N_STRING_DESCRIPTORS) {
        return NULL;
    }
    return Standard_GetDescriptorData(length, &String_Descriptor[wValue0]);
}

static void usbSetConfiguration(void) {
    if (pInformation->Current_Configuration != 0) {
        USBLIB->state = USB_CONFIGURED;
    }
}

static void usbSetDeviceAddress(void) {
    USBLIB->state = USB_ADDRESSED;
}

