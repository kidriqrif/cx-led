/* effects.h - the effect, colour and speed tables behind the UI. */
#ifndef CXLED_EFFECTS_H
#define CXLED_EFFECTS_H

#include <stdbool.h>
#include <stdint.h>

#include "led.h"

enum effect_type { FX_OFF, FX_MONO, FX_DUO, FX_CUSTOM };

typedef struct {
	const char *name;
	const char *key;        /* name in settings and preset files */
	enum effect_type type;
	uint32_t green, red;    /* FX_MONO: the shape is in green, red unused */
} effect_t;

typedef struct {
	const char *name;
	const char *key;
	uint8_t green, red;     /* duty in 1/32: 0, 4, 8, 16 or 32 */
	uint16_t rgb;           /* on-screen preview colour */
} colour_t;

typedef struct {
	const char *name;
	const char *key;
	uint32_t step;          /* LED clock ticks per step */
} speed_t;

enum { EFFECT_OFF = 0, EFFECT_SOLID = 1 };
enum { COL_GREEN = 0, COL_RED = 1, COL_YELLOW = 2 };
enum { SPEED_DEFAULT = 3 };

/* Step length used to dim a channel by pulse-width modulation: 16 ticks is
 * about 0.5 ms, so even a 1-in-8 pulse train repeats at 256 Hz. */
#define PWM_STEP 16u

extern const effect_t effects[];
extern const colour_t colours[];
extern const speed_t speeds[];
extern const int n_effects, n_colours, n_speeds;

/* What the user has picked. */
typedef struct {
	int effect;             /* index into effects[] */
	int colour;             /* index into colours[] */
	int speed;              /* index into speeds[] */
	uint32_t custom_green;  /* masks for the Custom effect */
	uint32_t custom_red;
} selection_t;

void selection_default(selection_t *s);
void selection_fix(selection_t *s);         /* clamp indexes, drop PWM colours from blink effects */
void selection_pattern(const selection_t *s, led_pattern_t *out);
void selection_apply(const selection_t *s); /* program the LED */

/* The masks with PWM colours rounded to plain ones: where the pattern
 * editor starts from. */
void selection_masks(const selection_t *s, uint32_t *green, uint32_t *red);

bool selection_uses_colour(const selection_t *s);
bool selection_uses_speed(const selection_t *s);
bool selection_is_pwm(const selection_t *s);

bool colour_is_mixed(int colour);           /* needs PWM, so only with Solid */
int colour_next(int colour, int dir, bool allow_mixed);
int effect_find(const char *key);
int colour_find(const char *key);
int speed_find(const char *key);

uint32_t pwm_mask(unsigned duty);           /* duty in 1/32, spread evenly */

#endif
