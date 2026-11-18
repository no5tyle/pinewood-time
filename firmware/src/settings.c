#include "settings.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/flash.h"

/* The last 4 KB sector of flash, well clear of the firmware */
#define SETTINGS_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC 0x50544D33u /* "PTM3": with each lane's start gate */
#define SETTINGS_MAGIC_V2 0x50544D32u /* "PTM2": firmware 1.1, before start gates (still read) */

typedef struct {
    uint32_t magic;
    pt_config cfg;
    uint32_t check;
} stored;

/* Firmware 1.1's settings, so an upgrade keeps them */
typedef struct {
    uint8_t num_lanes;
    bool lane_trip_high;
    bool gate_trip_high;
    uint8_t brightness;
    bool button_next;
    uint8_t button_key;
} config_v2;
typedef struct {
    uint32_t magic;
    config_v2 cfg;
    uint32_t check;
} stored_v2;

static uint32_t checksum(const void *data, size_t size) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t h = 2166136261u; /* FNV-1a */
    for (size_t i = 0; i < size; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

pt_config settings_load(void) {
    const stored *s = (const stored *)(XIP_BASE + SETTINGS_OFFSET);
    if (s->magic == SETTINGS_MAGIC && s->check == checksum(&s->cfg, sizeof s->cfg) && s->cfg.num_lanes >= 1 &&
        s->cfg.num_lanes <= PT_MAX_LANES)
        return s->cfg;
    const stored_v2 *old = (const stored_v2 *)(XIP_BASE + SETTINGS_OFFSET);
    if (old->magic == SETTINGS_MAGIC_V2 && old->check == checksum(&old->cfg, sizeof old->cfg) &&
        old->cfg.num_lanes >= 1 && old->cfg.num_lanes <= 8) {
        /* every lane on gate 1, as before */
        pt_config c = pt_default_config();
        c.num_lanes = old->cfg.num_lanes;
        c.lane_trip_high = old->cfg.lane_trip_high;
        c.gate_trip_high = old->cfg.gate_trip_high;
        c.brightness = old->cfg.brightness;
        c.button_next = old->cfg.button_next;
        c.button_key = old->cfg.button_key;
        return c;
    }
    return pt_default_config();
}

static void write_sector(void *param) {
    const uint8_t *page = (const uint8_t *)param;
    flash_range_erase(SETTINGS_OFFSET, FLASH_SECTOR_SIZE);
    flash_range_program(SETTINGS_OFFSET, page, FLASH_PAGE_SIZE);
}

void settings_save(const pt_config *cfg) {
    static uint8_t page[FLASH_PAGE_SIZE];
    memset(page, 0xFF, sizeof page);
    stored s = {.magic = SETTINGS_MAGIC, .cfg = *cfg, .check = checksum(cfg, sizeof *cfg)};
    memcpy(page, &s, sizeof s);
    /* Writing flash stops the code running from it, so nothing else may run meanwhile */
    flash_safe_execute(write_sector, page, 1000);
}
