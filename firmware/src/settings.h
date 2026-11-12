/* The timer's settings (lanes, sensor polarity, display brightness), kept in the Pico's flash */
#ifndef PT_SETTINGS_H
#define PT_SETTINGS_H

#include "timer_core.h"

/* The saved settings, or the defaults if none have been saved */
pt_config settings_load(void);
void settings_save(const pt_config *cfg);

#endif
