/* The USB connection: a serial port for the race software, and a keyboard for the reset button's key */
#ifndef PT_USB_H
#define PT_USB_H

#include <stdbool.h>
#include <stdint.h>

void usb_init(void);
/* Keep USB running: call this often (every pass of the main loop) */
void usb_task(void);
/* Whether software has the serial port open (it raised DTR, or has sent something) */
bool usb_host_listening(void);
/* Text to the serial port (dropped if nobody has it open) */
void usb_write(const char *text);
/* The next character from the serial port, or -1 */
int usb_read(void);
/* Types one key (a USB keyboard usage code) on the keyboard */
void usb_press_key(uint8_t code);

#endif
