/* TinyUSB settings: the timer is a USB serial port (CDC) and a keyboard (HID) on one cable */
#ifndef PT_TUSB_CONFIG_H
#define PT_TUSB_CONFIG_H

#define CFG_TUSB_RHPORT0_MODE OPT_MODE_DEVICE
#define CFG_TUD_ENDPOINT0_SIZE 64

#define CFG_TUD_CDC 1
#define CFG_TUD_HID 1
#define CFG_TUD_MSC 0
#define CFG_TUD_MIDI 0
#define CFG_TUD_VENDOR 0

/* Room for a whole #info reply or a full set of results without waiting */
#define CFG_TUD_CDC_RX_BUFSIZE 256
#define CFG_TUD_CDC_TX_BUFSIZE 1024
#define CFG_TUD_CDC_EP_BUFSIZE 64
#define CFG_TUD_HID_EP_BUFSIZE 8

#endif
