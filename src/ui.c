#include <libndls.h>
#include <stdio.h>
#include <string.h>

#include "effects.h"
#include "gfx.h"
#include "led.h"
#include "ui.h"

#define C_BG      RGB( 18,  20,  26)
#define C_BAR     RGB( 28,  31,  40)
#define C_SEL     RGB( 44,  50,  66)
#define C_LINE    RGB( 52,  56,  68)
#define C_ACCENT  RGB( 90, 170, 255)
#define C_TEXT    RGB(232, 234, 240)
#define C_DIM     RGB(128, 134, 148)
#define C_FAINT   RGB( 72,  76,  88)
#define C_UNLIT   RGB( 46,  50,  60)
#define C_WHITE   RGB(255, 255, 255)

/* The loop sleeps FRAME_MS and drawing takes a few ms more, so the on-screen
 * preview assumes 25 ms per frame. It drifts; the real LED does not. */
#define FRAME_MS      20
#define FRAME_TICKS   (25 * LED_HZ / 1000)
#define REPEAT_DELAY  16    /* frames before a held key repeats */
#define REPEAT_RATE   3

/* ---- keys ---- */

enum {
	K_UP, K_DOWN, K_LEFT, K_RIGHT, K_PLUS, K_MINUS,  /* repeat when held */
	K_ENTER, K_ESC, K_MENU, K_G, K_R, K_Y, K_OFF, K_DEL, K_CLEAR, K_U,
	K_COUNT
};
#define K_LAST_REPEAT K_MINUS

static bool key_down(int k)
{
	switch (k) {
	case K_UP:    return isKeyPressed(KEY_NSPIRE_UP) || isKeyPressed(KEY_NSPIRE_8);
	case K_DOWN:  return isKeyPressed(KEY_NSPIRE_DOWN) || isKeyPressed(KEY_NSPIRE_2);
	case K_LEFT:  return isKeyPressed(KEY_NSPIRE_LEFT) || isKeyPressed(KEY_NSPIRE_4);
	case K_RIGHT: return isKeyPressed(KEY_NSPIRE_RIGHT) || isKeyPressed(KEY_NSPIRE_6);
	case K_PLUS:  return isKeyPressed(KEY_NSPIRE_PLUS);
	case K_MINUS: return isKeyPressed(KEY_NSPIRE_MINUS);
	case K_ENTER: return isKeyPressed(KEY_NSPIRE_ENTER) || isKeyPressed(KEY_NSPIRE_RET)
	                     || isKeyPressed(KEY_NSPIRE_CLICK);
	case K_ESC:   return isKeyPressed(KEY_NSPIRE_ESC);
	case K_MENU:  return isKeyPressed(KEY_NSPIRE_MENU);
	case K_G:     return isKeyPressed(KEY_NSPIRE_G);
	case K_R:     return isKeyPressed(KEY_NSPIRE_R);
	case K_Y:     return isKeyPressed(KEY_NSPIRE_Y);
	case K_OFF:   return isKeyPressed(KEY_NSPIRE_0) || isKeyPressed(KEY_NSPIRE_PERIOD)
	                     || isKeyPressed(KEY_NSPIRE_SPACE);
	case K_DEL:   return isKeyPressed(KEY_NSPIRE_DEL);
	case K_CLEAR: return isKeyPressed(KEY_NSPIRE_C);
	case K_U:     return isKeyPressed(KEY_NSPIRE_U);
	}
	return false;
}

/* Frames each key has been held for, and keys to ignore until released. */
static unsigned held[K_COUNT];
static bool stale[K_COUNT];

/* Called when a screen opens or closes: the keys held now do nothing until
 * they are released, so the key that switched screens does not act again. */
static void keys_settle(void)
{
	for (int k = 0; k < K_COUNT; k++)
		stale[k] = key_down(k);
}

/* The key just pressed, or held long enough to repeat; -1 if none. A key
 * pressed in the same frame as another one is reported in the next frame. */
static int key_event(void)
{
	int ev = -1;

	for (int k = 0; k < K_COUNT; k++) {
		if (!key_down(k)) {
			held[k] = 0;
			stale[k] = false;
			continue;
		}
		if (stale[k] || (ev >= 0 && held[k] == 0))
			continue;
		held[k]++;
		if (ev < 0 && (held[k] == 1 || (k <= K_LAST_REPEAT && held[k] > REPEAT_DELAY
		                                && (held[k] - REPEAT_DELAY) % REPEAT_RATE == 0)))
			ev = k;
	}
	return ev;
}

/* True if the event key_event() just reported for k is a repeat. */
static bool key_repeated(int k)
{
	return held[k] > 1;
}

/* ---- drawing ---- */

static uint16_t state_colour(bool g, bool r)
{
	if (g && r)
		return colours[COL_YELLOW].rgb;
	if (g)
		return colours[COL_GREEN].rgb;
	if (r)
		return colours[COL_RED].rgb;
	return C_UNLIT;
}

/* What the LED shows at a step. A PWM colour is drawn as the mix it makes. */
static uint16_t step_colour(const selection_t *s, const led_pattern_t *p, unsigned step, bool *lit)
{
	bool g = led_bit(p->green, step), r = led_bit(p->red, step);

	if (selection_is_pwm(s)) {
		*lit = true;
		return colours[s->colour].rgb;
	}
	*lit = g || r;
	return state_colour(g, r);
}

static void draw_led(uint16_t c, bool lit)
{
	const int x = GFX_W - 28, y = 20;

	if (lit) {
		gfx_disc(x, y, 16, gfx_mix(C_BAR, c, 48));
		gfx_disc(x, y, 13, gfx_mix(C_BAR, c, 110));
	}
	gfx_disc(x, y, 10, c);
	gfx_disc(x - 3, y - 3, 3, gfx_mix(c, C_WHITE, lit ? 150 : 40));
}

static void draw_header(const char *title)
{
	gfx_rect(0, 0, GFX_W, 40, C_BAR);
	gfx_rect(0, 40, GFX_W, 1, C_LINE);
	gfx_text(14, 4, title, C_TEXT, 2);
}

static void draw_footer(const char *text)
{
	gfx_text((GFX_W - gfx_text_width(text, 1)) / 2, GFX_H - 18, text, C_DIM, 1);
}

static int step_state(const selection_t *s, int i)
{
	return led_bit(s->custom_green, i) | led_bit(s->custom_red, i) << 1;
}

static void set_step(selection_t *s, int i, int state)
{
	uint32_t bit = 0x80000000u >> i;

	s->custom_green = state & 1 ? s->custom_green | bit : s->custom_green & ~bit;
	s->custom_red = state & 2 ? s->custom_red | bit : s->custom_red & ~bit;
}

/* ---- register view ---- */

static const char *hw_name(void)
{
	if (is_classic)
		return "TI-Nspire";
	if (is_cx2)
		return "TI-Nspire CX II";
	if (is_cm)
		return "TI-Nspire CM";
	return "TI-Nspire CX";
}

static void info_line(int *y, const char *label, const char *value)
{
	gfx_text(16, *y, label, C_DIM, 1);
	gfx_text(128, *y, value, C_TEXT, 1);
	*y += 18;
}

/* Hackspire: "possibly another peripheral bus access disable register". */
#define GATE2 (*(volatile uint32_t *)0x900B0020u)

static void gate_line(int *y, const char *label, uint32_t gate)
{
	char buf[24];

	snprintf(buf, sizeof(buf), "0x%08X %s", (unsigned)gate, gate & LED_BUS_BIT ? "shut" : "open");
	info_line(y, label, buf);
}

/* Also a test bench for when the LED stays dark: G R Y 0 force a steady
 * colour, U opens the second bus gate and applies p again. On the way out p
 * goes back on the LED, before the gate closes. Returns true if p was
 * applied again. */
static bool info_screen(const led_pattern_t *p)
{
	int forced = -1;  /* 1 green, 2 red, 3 yellow, 0 off; -1 if none */
	bool probed = false, gate2_open = false;
	uint32_t gate2_saved = 0;

	keys_settle();
	for (;;) {
		int k = key_event();
		int colour = k == K_G ? 1 : k == K_R ? 2 : k == K_Y ? 3 : k == K_OFF ? 0 : -1;
		led_regs_t r;
		char buf[32];
		int y = 50;

		if (k == K_ESC || k == K_ENTER || k == K_MENU)
			break;
		if (colour >= 0) {
			forced = colour;
			led_force(true, colour & 1);
			led_force(false, colour & 2);
			probed = true;
		} else if (k == K_U && !is_cx2) {
			if (!gate2_open)
				gate2_saved = GATE2;
			GATE2 = gate2_open ? gate2_saved : 0;
			gate2_open = !gate2_open;
			forced = -1;
			led_apply(p);
			probed = true;
		}
		led_read(&r);
		gfx_clear(C_BG);
		draw_header("LED info");
		if (forced >= 0)
			draw_led(state_colour(forced & 1, forced & 2), forced != 0);
		info_line(&y, "Hardware", hw_name());
		snprintf(buf, sizeof(buf), "r%u", nl_ndless_rev());
		info_line(&y, "Ndless", buf);
		if (is_cx2) {
			info_line(&y, "Bus gates", "not on the CX II");
		} else {
			gate_line(&y, "Gate at start", led_bus_gate_at_start());
			gate_line(&y, "Gate now", LED_BUS_GATE);
			if (gate2_open)
				snprintf(buf, sizeof(buf), "0x%X to 0x%X", (unsigned)gate2_saved, (unsigned)GATE2);
			else
				snprintf(buf, sizeof(buf), "0x%08X", (unsigned)GATE2);
			info_line(&y, gate2_open ? "Gate 2 (U)" : "Gate 2", buf);
		}
		info_line(&y, "Writes stick", led_readback_ok() ? "yes" : "no");
		snprintf(buf, sizeof(buf), "0x%08X", (unsigned)r.ctrl);
		info_line(&y, "Control", buf);
		snprintf(buf, sizeof(buf), "0x%08X, step %u", (unsigned)r.g_mask, (unsigned)-r.g_delay);
		info_line(&y, "Green", buf);
		snprintf(buf, sizeof(buf), "0x%08X, step %u", (unsigned)r.r_mask, (unsigned)-r.r_delay);
		info_line(&y, "Red", buf);
		draw_footer(is_cx2 ? "G R Y 0: test LED  esc: back"
		                   : "G R Y 0: test LED  U: gate 2  esc: back");
		gfx_present();
		msleep(FRAME_MS);
	}
	if (probed)
		led_apply(p);
	if (gate2_open)
		GATE2 = gate2_saved;
	keys_settle();
	return probed;
}

/* ---- pattern editor ---- */

#define ED_X     16
#define ED_Y     72
#define ED_PITCH 18

static void draw_editor(const selection_t *s, const led_pattern_t *p, int cur, unsigned playing)
{
	static const char *const help[][2] = {
		{ "G R Y", "set a step" },          { "0 .", "off" },
		{ SYM_LEFT " " SYM_RIGHT, "move" }, { SYM_UP " " SYM_DOWN, "change" },
		{ "+ -", "speed" },                 { "C", "clear all" },
		{ "enter", "keep" },                { "esc", "cancel" },
	};
	static const char *const names[] = { "off", "green", "red", "yellow" };
	char buf[32];
	bool lit;
	uint16_t c = step_colour(s, p, playing, &lit);

	gfx_clear(C_BG);
	draw_header("Edit pattern");
	draw_led(c, lit);

	snprintf(buf, sizeof(buf), "Step %d: %s", cur + 1, names[step_state(s, cur)]);
	gfx_text(16, 48, buf, C_TEXT, 1);
	snprintf(buf, sizeof(buf), "cycle %s", speeds[s->speed].name);
	gfx_text(GFX_W - 16 - gfx_text_width(buf, 1), 48, buf, C_DIM, 1);

	for (int i = 0; i < LED_STEPS; i++) {
		int x = ED_X + (i % 16) * ED_PITCH, y = ED_Y + (i / 16) * 40;
		int st = step_state(s, i);

		gfx_rect(x, y, ED_PITCH - 2, 26, state_colour(st & 1, st & 2));
		if (i == cur) {
			gfx_frame(x - 2, y - 2, ED_PITCH + 2, 30, C_TEXT);
			gfx_frame(x - 1, y - 1, ED_PITCH, 28, C_TEXT);
		}
		if ((unsigned)i == playing)
			gfx_rect(x, y + 30, ED_PITCH - 2, 2, C_ACCENT);
	}

	for (int i = 0; i < 8; i++) {
		int x = i % 2 ? 168 : 16, y = 152 + i / 2 * 18;
		gfx_text(x, y, help[i][0], C_ACCENT, 1);
		gfx_text(x + 56, y, help[i][1], C_DIM, 1);
	}
}

/* Edits the Custom effect, starting from the current effect's steps: a mixed
 * Solid colour starts as plain green, red or yellow, and Off keeps the last
 * custom pattern. The LED follows every change; Esc puts the old selection
 * back. */
static void edit_pattern(selection_t *s)
{
	selection_t old = *s;
	led_pattern_t p;
	uint32_t ticks = 0;
	int cur = 0;

	if (effects[s->effect].type != FX_OFF)
		selection_masks(s, &s->custom_green, &s->custom_red);
	s->effect = effect_find("custom");
	selection_pattern(s, &p);
	led_apply(&p);
	keys_settle();

	for (;;) {
		int k = key_event();
		int st = step_state(s, cur);
		bool changed = true;

		switch (k) {
		case K_ESC:
			*s = old;
			/* fall through */
		case K_ENTER:
			keys_settle();
			return;
		case K_LEFT:
			cur = (cur + LED_STEPS - 1) % LED_STEPS;
			changed = false;
			break;
		case K_RIGHT:
			cur = (cur + 1) % LED_STEPS;
			changed = false;
			break;
		case K_UP:
			set_step(s, cur, (st + 1) & 3);
			break;
		case K_DOWN:
			set_step(s, cur, (st + 3) & 3);
			break;
		case K_G:
		case K_R:
		case K_Y:
		case K_OFF:
			set_step(s, cur, k == K_G ? 1 : k == K_R ? 2 : k == K_Y ? 3 : 0);
			cur = (cur + 1) % LED_STEPS;
			break;
		case K_DEL:
			cur = (cur + LED_STEPS - 1) % LED_STEPS;
			set_step(s, cur, 0);
			break;
		case K_CLEAR:
			s->custom_green = 0;
			s->custom_red = 0;
			break;
		case K_PLUS:
			changed = s->speed > 0;
			if (changed)
				s->speed--;
			break;
		case K_MINUS:
			changed = s->speed < n_speeds - 1;
			if (changed)
				s->speed++;
			break;
		default:
			changed = false;
		}
		if (changed) {
			selection_pattern(s, &p);
			led_apply(&p);
			ticks = 0;
		}
		draw_editor(s, &p, cur, ticks / p.step);
		gfx_present();
		msleep(FRAME_MS);
		ticks = (ticks + FRAME_TICKS) % (p.step * LED_STEPS);
	}
}

/* ---- menu ---- */

enum { ROW_EFFECT, ROW_COLOUR, ROW_CYCLE, ROW_EDIT, ROW_STARTUP, ROW_INFO, ROW_COUNT };
#define ROW_Y 48
#define ROW_H 22
#define TL_Y  186

static bool row_enabled(const selection_t *s, int row)
{
	if (row == ROW_COLOUR)
		return selection_uses_colour(s);
	if (row == ROW_CYCLE)
		return selection_uses_speed(s);
	return true;
}

static int row_move(const selection_t *s, int row, int dir)
{
	do
		row = (row + dir + ROW_COUNT) % ROW_COUNT;
	while (!row_enabled(s, row));
	return row;
}

/* A menu line: a label, and either a value with arrows (choice) or a
 * pointer to another screen (value NULL). */
static void draw_row(int row, bool sel, bool enabled, const char *label,
                     const char *value, const uint16_t *swatch)
{
	int y = ROW_Y + row * ROW_H;
	uint16_t fg = enabled ? C_TEXT : C_FAINT;
	int x;

	if (sel) {
		gfx_rect(8, y, GFX_W - 16, ROW_H - 2, C_SEL);
		gfx_rect(8, y, 3, ROW_H - 2, C_ACCENT);
	}
	gfx_text(20, y + 2, label, fg, 1);
	if (!value) {
		gfx_text(GFX_W - 24, y + 2, SYM_RIGHT, sel ? C_ACCENT : C_FAINT, 1);
		return;
	}
	x = GFX_W - 30 - gfx_text_width(value, 1);
	gfx_text(x, y + 2, value, fg, 1);
	if (swatch && enabled) {
		gfx_disc(x - 10, y + 10, 5, *swatch);
		x -= 16;
	}
	if (sel && enabled) {
		gfx_text(x - 14, y + 2, SYM_LEFT, C_ACCENT, 1);
		gfx_text(GFX_W - 24, y + 2, SYM_RIGHT, C_ACCENT, 1);
	}
}

static void draw_timeline(const selection_t *s, const led_pattern_t *p, int playing)
{
	char buf[32];

	if (effects[s->effect].type == FX_OFF)
		strcpy(buf, "LED off");
	else if (selection_is_pwm(s))
		strcpy(buf, "steady, dimmed by PWM");
	else if (s->effect == EFFECT_SOLID)
		strcpy(buf, "steady");
	else {
		unsigned us = p->step * 15625u / 512u;  /* ticks * 1e6 / 32768 */
		snprintf(buf, sizeof(buf), "1 step = %u.%u ms", us / 1000, us % 1000 / 100);
	}
	gfx_text(16, TL_Y, "Pattern", C_DIM, 1);
	gfx_text(GFX_W - 16 - gfx_text_width(buf, 1), TL_Y, buf, C_DIM, 1);

	for (int i = 0; i < LED_STEPS; i++) {
		bool lit;
		gfx_rect(16 + i * 9, TL_Y + 20, 8, 12, step_colour(s, p, i, &lit));
	}
	if (playing >= 0)
		gfx_rect(16 + playing * 9, TL_Y + 34, 8, 2, C_TEXT);
}

static void draw_menu(const selection_t *s, bool startup, int row, const led_pattern_t *p, unsigned playing)
{
	bool use_colour = selection_uses_colour(s), use_speed = selection_uses_speed(s);
	bool lit;
	uint16_t c = step_colour(s, p, playing, &lit);

	gfx_clear(C_BG);
	draw_header("cx-led");
	draw_led(c, lit);

	draw_row(ROW_EFFECT, row == ROW_EFFECT, true, "Effect", effects[s->effect].name, NULL);
	draw_row(ROW_COLOUR, row == ROW_COLOUR, use_colour, "Colour",
	         use_colour ? colours[s->colour].name : "-", &colours[s->colour].rgb);
	draw_row(ROW_CYCLE, row == ROW_CYCLE, use_speed, "Cycle",
	         use_speed ? speeds[s->speed].name : "-", NULL);
	draw_row(ROW_EDIT, row == ROW_EDIT, true, "Edit pattern", NULL, NULL);
	draw_row(ROW_STARTUP, row == ROW_STARTUP, true, "Run at startup", startup ? "On" : "Off", NULL);
	draw_row(ROW_INFO, row == ROW_INFO, true, "LED info", NULL, NULL);

	draw_timeline(s, p, s->effect == EFFECT_SOLID || effects[s->effect].type == FX_OFF ? -1 : (int)playing);
	draw_footer("esc: exit, the LED keeps running");
}

/* Returns true if the selection changed. */
static bool row_action(selection_t *s, bool *startup, int row, int k)
{
	int dir = k == K_LEFT ? -1 : 1;

	switch (row) {
	case ROW_EFFECT:
		if (k == K_ENTER)
			return false;
		s->effect = (s->effect + dir + n_effects) % n_effects;
		selection_fix(s);
		return true;
	case ROW_COLOUR:
		if (k == K_ENTER)
			return false;
		s->colour = colour_next(s->colour, dir, s->effect == EFFECT_SOLID);
		return true;
	case ROW_CYCLE:
		if (k == K_ENTER || s->speed + dir < 0 || s->speed + dir >= n_speeds)
			return false;
		s->speed += dir;
		return true;
	/* These act once per press, not again while left or right is held. */
	case ROW_EDIT:
		if (k == K_LEFT || key_repeated(k))
			return false;
		edit_pattern(s);
		return true;
	case ROW_STARTUP:
		if (!key_repeated(k))
			*startup = !*startup;
		return false;
	}
	return false;
}

bool ui_run(selection_t *s, bool *startup)
{
	led_pattern_t p;
	uint32_t ticks = 0;
	int row = ROW_EFFECT;

	if (!gfx_init())
		return false;
	selection_pattern(s, &p);
	keys_settle();

	for (;;) {
		int k = key_event();

		if (k == K_ESC)
			break;
		if (k == K_UP || k == K_DOWN) {
			row = row_move(s, row, k == K_UP ? -1 : 1);
		} else if (k == K_MENU || (row == ROW_INFO && (k == K_RIGHT || k == K_ENTER)
		                            && !key_repeated(k))) {
			if (info_screen(&p))
				ticks = 0;
		} else if ((k == K_LEFT || k == K_RIGHT || k == K_ENTER) && row_action(s, startup, row, k)) {
			selection_pattern(s, &p);
			led_apply(&p);
			ticks = 0;
		}
		draw_menu(s, *startup, row, &p, ticks / p.step);
		gfx_present();
		msleep(FRAME_MS);
		ticks = (ticks + FRAME_TICKS) % (p.step * LED_STEPS);
	}
	gfx_quit();
	wait_no_key_pressed();  /* or the held Esc closes the next message box */
	return true;
}
