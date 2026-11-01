#include <string.h>

#include "effects.h"
#include "gfx.h"

/* Masks are played from bit 31 down to bit 0, one bit per step. */
const effect_t effects[] = {
	{ "Off",           "off",       FX_OFF,    0,          0 },
	{ "Solid",         "solid",     FX_MONO,   0xFFFFFFFF, 0 },
	{ "Blink",         "blink",     FX_MONO,   0xFFFF0000, 0 },
	{ "Fast blink",    "fast",      FX_MONO,   0xF0F0F0F0, 0 },
	{ "Strobe",        "strobe",    FX_MONO,   0xAAAAAAAA, 0 },
	{ "Beacon",        "beacon",    FX_MONO,   0x80000000, 0 },
	{ "Double flash",  "double",    FX_MONO,   0xCC000000, 0 },
	{ "Triple flash",  "triple",    FX_MONO,   0xCCC00000, 0 },
	{ "Heartbeat",     "heartbeat", FX_MONO,   0xE8000000, 0 },
	/* S (5) + gap (3) + O (11) + gap (3) + S (5) + word gap (5) = 32 */
	{ "SOS",           "sos",       FX_MONO,   0xA8EEE2A0, 0 },
	{ "Alternate R/G", "alternate", FX_DUO,    0xFFFF0000, 0x0000FFFF },
	/* green 14 steps, yellow 4, red 14 */
	{ "Traffic light", "traffic",   FX_DUO,    0xFFFFC000, 0x0003FFFF },
	/* green, yellow, red, off: 8 steps each */
	{ "Colour cycle",  "cycle",     FX_DUO,    0xFFFF0000, 0x00FFFF00 },
	{ "Emergency",     "emergency", FX_DUO,    0x00A000A0, 0xA000A000 },
	{ "Custom",        "custom",    FX_CUSTOM, 0,          0 },
};
const int n_effects = sizeof(effects) / sizeof(effects[0]);

const colour_t colours[] = {
	{ "Green",       "green",     32,  0, RGB( 40, 230,  90) },
	{ "Red",         "red",        0, 32, RGB(255,  50,  40) },
	{ "Yellow",      "yellow",    32, 32, RGB(255, 220,  40) },
	{ "Orange",      "orange",     8, 32, RGB(255, 130,  20) },
	{ "Amber",       "amber",     16, 32, RGB(255, 175,  30) },
	{ "Lime",        "lime",      32,  8, RGB(170, 240,  40) },
	{ "Dim green",   "dimgreen",   4,  0, RGB( 20, 115,  45) },
	{ "Dim red",     "dimred",     0,  4, RGB(128,  25,  20) },
	{ "Dim yellow",  "dimyellow",  4,  4, RGB(128, 110,  20) },
};
const int n_colours = sizeof(colours) / sizeof(colours[0]);

/* Names give the time for one pass of the 32 steps at 32768 Hz. */
const speed_t speeds[] = {
	{ "0.25 s", "0.25",   256 },
	{ "0.5 s",  "0.5",    512 },
	{ "1 s",    "1",     1024 },
	{ "2 s",    "2",     2048 },
	{ "4 s",    "4",     4096 },
	{ "8 s",    "8",     8192 },
	{ "16 s",   "16",   16384 },
	{ "32 s",   "32",   32768 },
};
const int n_speeds = sizeof(speeds) / sizeof(speeds[0]);

uint32_t pwm_mask(unsigned duty)
{
	uint32_t mask = 0;
	if (duty >= 32)
		return 0xFFFFFFFF;
	for (unsigned i = 0; i < 32; i++)
		if ((i * duty) % 32 < duty)
			mask |= 0x80000000u >> i;
	return mask;
}

bool colour_is_mixed(int colour)
{
	const colour_t *c = &colours[colour];
	return (c->green % 32) || (c->red % 32);
}

int colour_next(int colour, int dir, bool allow_mixed)
{
	for (int i = 0; i < n_colours; i++) {
		colour = (colour + dir + n_colours) % n_colours;
		if (allow_mixed || !colour_is_mixed(colour))
			return colour;
	}
	return COL_GREEN;
}

/* The plain colour closest to a PWM one. */
static int colour_base(int colour)
{
	const colour_t *c = &colours[colour];
	if (c->green == c->red)
		return COL_YELLOW;
	return c->green > c->red ? COL_GREEN : COL_RED;
}

void selection_default(selection_t *s)
{
	s->effect = EFFECT_OFF;
	s->colour = COL_GREEN;
	s->speed = SPEED_DEFAULT;
	s->custom_green = 0xFF000000;
	s->custom_red = 0x0000FF00;
}

void selection_fix(selection_t *s)
{
	if (s->effect < 0 || s->effect >= n_effects)
		s->effect = EFFECT_OFF;
	if (s->colour < 0 || s->colour >= n_colours)
		s->colour = COL_GREEN;
	if (s->speed < 0 || s->speed >= n_speeds)
		s->speed = SPEED_DEFAULT;
	if (s->effect != EFFECT_SOLID && colour_is_mixed(s->colour))
		s->colour = colour_base(s->colour);
}

bool selection_uses_colour(const selection_t *s)
{
	return effects[s->effect].type == FX_MONO;
}

bool selection_uses_speed(const selection_t *s)
{
	return effects[s->effect].type != FX_OFF && s->effect != EFFECT_SOLID;
}

bool selection_is_pwm(const selection_t *s)
{
	return s->effect == EFFECT_SOLID && colour_is_mixed(s->colour);
}

void selection_pattern(const selection_t *s, led_pattern_t *out)
{
	const effect_t *e = &effects[s->effect];
	const colour_t *c = &colours[s->colour];

	out->step = speeds[s->speed].step;
	switch (e->type) {
	case FX_OFF:
		out->green = 0;
		out->red = 0;
		break;
	case FX_CUSTOM:
		out->green = s->custom_green;
		out->red = s->custom_red;
		break;
	case FX_DUO:
		out->green = e->green;
		out->red = e->red;
		break;
	case FX_MONO:
		if (s->effect == EFFECT_SOLID) {
			out->green = pwm_mask(c->green);
			out->red = pwm_mask(c->red);
			if (colour_is_mixed(s->colour))
				out->step = PWM_STEP;
		} else {
			out->green = c->green ? e->green : 0;
			out->red = c->red ? e->green : 0;
		}
		break;
	}
}

void selection_apply(const selection_t *s)
{
	led_pattern_t p;
	selection_pattern(s, &p);
	led_apply(&p);
}

void selection_masks(const selection_t *s, uint32_t *green, uint32_t *red)
{
	selection_t plain = *s;
	led_pattern_t p;
	if (colour_is_mixed(plain.colour))
		plain.colour = colour_base(plain.colour);
	selection_pattern(&plain, &p);
	*green = p.green;
	*red = p.red;
}

int effect_find(const char *key)
{
	for (int i = 0; i < n_effects; i++)
		if (!strcmp(key, effects[i].key))
			return i;
	return -1;
}

int colour_find(const char *key)
{
	for (int i = 0; i < n_colours; i++)
		if (!strcmp(key, colours[i].key))
			return i;
	return -1;
}

int speed_find(const char *key)
{
	for (int i = 0; i < n_speeds; i++)
		if (!strcmp(key, speeds[i].key))
			return i;
	return -1;
}
