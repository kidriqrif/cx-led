/*
 * settings.h - the saved settings, preset files and the startup entry.
 *
 * Settings and presets are text files of key=value lines:
 *
 *   effect=blink        an effect key from effects.c
 *   colour=red          a colour key ("color" works too)
 *   cycle=2             seconds per pass: 0.25, 0.5, 1, 2, 4, 8, 16 or 32
 *   pattern=GGGG....    steps of the Custom effect: G, R, Y or . for off
 *
 * The OS lists only files whose names end in .tns, hence cxled.cfg.tns and
 * name.led.tns. Ndless runs this program when a .led.tns file is opened.
 */
#ifndef CXLED_SETTINGS_H
#define CXLED_SETTINGS_H

#include <stdbool.h>

#include "effects.h"

/* documents/ndless/cxled.cfg.tns. cx2_ok records that the user accepted the
 * CX II warning. Returns NULL on success, otherwise why it failed. */
const char *config_load(selection_t *s, bool *cx2_ok);
bool config_save(const selection_t *s, bool cx2_ok);

/* Reads a preset over *s: keys it leaves out keep their value. Returns NULL
 * on success, otherwise a message for the user, and then *s is unchanged. */
const char *preset_load(const char *path, selection_t *s);

/* documents/ndless/startup/cxled.led.tns, which Ndless opens at boot. */
bool startup_enabled(void);
bool startup_set(bool on, const selection_t *s);

/* Adds "ext.led=<this program>" to ndless.cfg.tns unless .led is taken.
 * Returns false if the program's file name has a space or '#' in it, which
 * that line cannot hold. */
bool register_presets(const char *argv0);

#endif
