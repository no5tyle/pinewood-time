#include "settings.h"

#include <string.h>

#include "hardware/flash.h"
#include "hardware/sync.h"
#include "pico/flash.h"

/* The last 4 KB sector of flash, well clear of the firmware */
#define SETTINGS_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
#define SETTINGS_MAGIC 0x50544D31u /* "PTM1" */

typedef struct {
    uint32_t magic;
    pt_config cfg;
    uint32_t check;
} stored;

static uint32_t checksum(const pt_config *cfg) {
    const uint8_t *p = (const uint8_t *)cfg;
    uint32_t h = 2166136261u; /* FNV-1a */
    for (size_t i = 0; i < sizeof *cfg; i++) h = (h ^ p[i]) * 16777619u;
    return h;
}

pt_config settings_load(void) {
    const stored *s = (const stored *)(XIP_BASE + SETTINGS_OFFSET);
    if (s->magic == SETTINGS_MAGIC && s->check == checksum(&s->cfg) && s->cfg.num_lanes >= 1 &&
        s->cfg.num_lanes <= PT_MAX_LANES)
        return s->cfg;
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
    stored s = {.magic = SETTINGS_MAGIC, .cfg = *cfg, .check = checksum(cfg)};
    memcpy(page, &s, sizeof s);
    /* Writing flash stops the code running from it, so nothing else may run meanwhile */
    flash_safe_execute(write_sector, page, 1000);
}
