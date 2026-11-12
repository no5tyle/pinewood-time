/*
 * USB: one serial port (CDC) and one keyboard (HID), run with TinyUSB.
 *
 * The race software talks to the timer over the serial port. The keyboard is only used when the reset
 * button is set to "#button next": each press types a key, for race software with a "next heat" shortcut.
 */
#include "usb.h"

#include <string.h>

#include "pico/bootrom.h"
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "tusb.h"

/* ---------------------------------------------------------------- Descriptors */
/* Raspberry Pi's vendor ID with the Pico SDK's serial-port product ID; the device version marks this
 * firmware's serial + keyboard layout, so computers don't reuse what they learned of the plain serial one */
#define USB_VID 0x2E8A
#if PICO_RP2040
#define USB_PID 0x000A
#else
#define USB_PID 0x0009
#endif
#define USB_BCD_DEVICE 0x0110

enum { ITF_CDC = 0, ITF_CDC_DATA, ITF_HID, ITF_COUNT };
#define EP_CDC_NOTIFY 0x81
#define EP_CDC_OUT 0x02
#define EP_CDC_IN 0x82
#define EP_HID_IN 0x83

enum { STR_LANG = 0, STR_MANUFACTURER, STR_PRODUCT, STR_SERIAL, STR_CDC, STR_HID };

static const tusb_desc_device_t device = {
    .bLength = sizeof(tusb_desc_device_t),
    .bDescriptorType = TUSB_DESC_DEVICE,
    .bcdUSB = 0x0200,
    /* several interfaces, grouped by an interface association */
    .bDeviceClass = TUSB_CLASS_MISC,
    .bDeviceSubClass = MISC_SUBCLASS_COMMON,
    .bDeviceProtocol = MISC_PROTOCOL_IAD,
    .bMaxPacketSize0 = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor = USB_VID,
    .idProduct = USB_PID,
    .bcdDevice = USB_BCD_DEVICE,
    .iManufacturer = STR_MANUFACTURER,
    .iProduct = STR_PRODUCT,
    .iSerialNumber = STR_SERIAL,
    .bNumConfigurations = 1,
};

static const uint8_t hid_report[] = {TUD_HID_REPORT_DESC_KEYBOARD()};

#define CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
static const uint8_t configuration[] = {
    TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 250),
    TUD_CDC_DESCRIPTOR(ITF_CDC, STR_CDC, EP_CDC_NOTIFY, 8, EP_CDC_OUT, EP_CDC_IN, 64),
    TUD_HID_DESCRIPTOR(ITF_HID, STR_HID, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid_report), EP_HID_IN,
                       CFG_TUD_HID_EP_BUFSIZE, 10),
};

uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&device; }

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return configuration;
}

uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) {
    (void)instance;
    return hid_report;
}

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;
    static uint16_t desc[33];
    char serial[2 * PICO_UNIQUE_BOARD_ID_SIZE_BYTES + 1];
    const char *s;
    switch (index) {
    case STR_LANG:
        desc[1] = 0x0409; /* English */
        desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | 4);
        return desc;
    case STR_MANUFACTURER: s = "no5tyle"; break;
    case STR_PRODUCT: s = "pinewood-time race timer"; break;
    case STR_SERIAL:
        pico_get_unique_board_id_string(serial, sizeof serial);
        s = serial;
        break;
    case STR_CDC: s = "pinewood-time serial"; break;
    case STR_HID: s = "pinewood-time button"; break;
    default: return NULL;
    }
    size_t n = strlen(s);
    if (n > 32) n = 32;
    for (size_t i = 0; i < n; i++) desc[1 + i] = (uint8_t)s[i];
    desc[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 * n + 2));
    return desc;
}

/* The keyboard has nothing to report when asked, and no lights to set */
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t *buffer,
                               uint16_t len) {
    (void)instance, (void)report_id, (void)type, (void)buffer, (void)len;
    return 0;
}

void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t type, uint8_t const *buffer,
                           uint16_t len) {
    (void)instance, (void)report_id, (void)type, (void)buffer, (void)len;
}

/* Opening the port at 1200 baud restarts the Pico ready for new firmware, as the Pico SDK's serial port
 * does (so the usual tools can update it without pressing BOOTSEL) */
void tud_cdc_line_coding_cb(uint8_t itf, cdc_line_coding_t const *coding) {
    (void)itf;
    if (coding->bit_rate == 1200) reset_usb_boot(0, 0);
}

/* ---------------------------------------------------------------- Using it */
void usb_init(void) { tusb_init(); }

static uint8_t key_waiting; /* to press */
static bool key_down;       /* pressed: release it next */

void usb_task(void) {
    tud_task();
    if (!tud_hid_ready()) return;
    if (key_down) {
        tud_hid_keyboard_report(0, 0, NULL);
        key_down = false;
    } else if (key_waiting) {
        uint8_t keys[6] = {key_waiting};
        if (tud_hid_keyboard_report(0, 0, keys)) {
            key_waiting = 0;
            key_down = true;
        }
    }
}

void usb_press_key(uint8_t code) {
    if (!code) return;
    key_waiting = code;
}

/* Someone has the port open: it raised DTR, or (for software that doesn't) it has sent us something since
 * the cable went in */
static bool heard_from_host;

void tud_umount_cb(void) { heard_from_host = false; }

bool usb_host_listening(void) { return tud_cdc_connected() || (tud_mounted() && heard_from_host); }

void usb_write(const char *text) {
    if (!tud_mounted()) return;
    bool listening = usb_host_listening();
    size_t left = strlen(text);
    absolute_time_t give_up = make_timeout_time_ms(200);
    while (left) {
        uint32_t n = tud_cdc_write(text, (uint32_t)left);
        text += n;
        left -= n;
        if (left) {
            /* the buffer's full: let USB send some, unless the computer has stopped reading */
            tud_cdc_write_flush();
            tud_task();
            /* nobody reading: drop the rest rather than hold up the timer */
            if (!listening || time_reached(give_up)) return;
        }
    }
    tud_cdc_write_flush();
}

int usb_read(void) {
    if (!tud_cdc_available()) return -1;
    uint8_t c;
    if (tud_cdc_read(&c, 1) != 1) return -1;
    heard_from_host = true;
    return c;
}
