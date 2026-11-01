/*
 * cx-led - sets the TI-Nspire CX status LED to a colour and flash pattern.
 *
 * The LED controller plays its pattern on its own, so the LED keeps going
 * after this program exits and while other programs run.
 *
 *   cxled.tns           opens the menu
 *   name.led.tns        (opened from My Documents) applies that preset
 *   ndless/startup/     cxled.led.tns there applies the setting at boot
 */
#include <libndls.h>
#include <stdio.h>
#include <string.h>

#include "effects.h"
#include "led.h"
#include "settings.h"
#include "ui.h"

#define TITLE "cx-led"
#define NDLESS_REV_MIN 2004  /* lcd_init() and lcd_blit() */

/* The LED is only known to work on the CX. A CX II needs the user's go-ahead
 * once; quiet runs (at boot) never ask and do nothing without it. */
static bool hardware_ok(bool *cx2_ok, bool quiet)
{
	if (led_supported() || *cx2_ok)
		return true;
	if (quiet)
		return false;
	*cx2_ok = show_msgbox_2b(TITLE,
		"cx-led was written for the TI-Nspire CX and has not been tested on a CX II, "
		"whose LED may work differently.\n\nTry it anyway?", "Try it", "Quit") == 1;
	return *cx2_ok;
}

static const char *base_name(const char *path)
{
	const char *p = strrchr(path, '/');
	return p ? p + 1 : path;
}

/* Saves the setting for the menu and, while startup is on, for startup. */
static void save_setting(const selection_t *s, bool cx2_ok, bool startup)
{
	if (!config_save(s, cx2_ok))
		show_msgbox(TITLE, "Could not save the settings to ndless/cxled.cfg.tns.");
	if (startup && !startup_set(true, s))
		show_msgbox(TITLE, "Could not write ndless/startup/cxled.led.tns.");
}

/* Applies a preset file over the saved settings. */
static void open_preset(const char *path, selection_t *s, bool *cx2_ok, bool quiet)
{
	const char *err = preset_load(path, s);

	if (err) {
		if (!quiet) {
			char msg[160];
			snprintf(msg, sizeof(msg), "%s\n\n%s", base_name(path), err);
			show_msgbox(TITLE, msg);
		}
		return;
	}
	if (!hardware_ok(cx2_ok, quiet))
		return;
	selection_apply(s);
	if (!quiet)  /* it becomes the current setting, like a change in the menu */
		save_setting(s, *cx2_ok, startup_enabled());
}

static void run_menu(const char *argv0, selection_t *s, bool *cx2_ok)
{
	bool startup = startup_enabled(), was_startup = startup;

	if (!hardware_ok(cx2_ok, false))
		return;
	if (!register_presets(argv0))
		show_msgbox(TITLE, "Presets cannot open with this program while its file name "
			"has a space or # in it. Rename it, for example to cxled.tns.");
	selection_apply(s);
	if (!ui_run(s, &startup)) {
		show_msgbox(TITLE, "Could not set up the screen.");
		return;
	}
	save_setting(s, *cx2_ok, startup);
	if (!startup && was_startup)
		startup_set(false, s);
}

int main(int argc, char *argv[])
{
	selection_t s;
	bool cx2_ok = false;

	assert_ndless_rev(NDLESS_REV_MIN);
	selection_default(&s);
	config_load(&s, &cx2_ok);  /* a missing file keeps the defaults */

	if (argc > 1)
		open_preset(argv[1], &s, &cx2_ok, nl_isstartup());
	else if (nl_isstartup()) {
		if (hardware_ok(&cx2_ok, true))
			selection_apply(&s);
	} else
		run_menu(argv[0], &s, &cx2_ok);
	return 0;
}
