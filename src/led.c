#include <libndls.h>

#include "led.h"

bool led_supported(void)
{
	/* The CX II has a new ASIC and 0x90110B00 is not documented there. */
	return !is_cx2;
}

static bool gate_seen;
static uint32_t gate_at_start;
static bool writable;
static unsigned waited_ms;

/* Opens bus access to the LED controller. Other peripherals' bits stay as
 * they are, and the bit is only ever cleared, never set. */
static void bus_open(void)
{
	uint32_t gate;

	if (is_cx2)  /* its power management is somewhere else */
		return;
	gate = LED_BUS_GATE;
	if (!gate_seen) {
		gate_seen = true;
		gate_at_start = gate;
	}
	if (gate & LED_BUS_BIT)
		LED_BUS_GATE = gate & ~LED_BUS_BIT;
}

uint32_t led_bus_gate_at_start(void)
{
	if (is_cx2)
		return 0;
	if (!gate_seen)
		return LED_BUS_GATE;
	return gate_at_start;
}

static uint32_t clamp_step(uint32_t step)
{
	if (step < LED_STEP_MIN)
		return LED_STEP_MIN;
	if (step > LED_STEP_MAX)
		return LED_STEP_MAX;
	return step;
}

static uint32_t channel_bits(uint32_t mask, uint32_t blink, uint32_t force_on, uint32_t force_off)
{
	if (mask == 0)
		return force_off;
	if (mask == 0xFFFFFFFFu)
		return force_on;
	return blink;
}

void led_apply(const led_pattern_t *p)
{
	uint32_t delay = -clamp_step(p->step);
	uint32_t keep;
	unsigned ms;

	bus_open();
	keep = LED_CTRL & ~LED_CTRL_KNOWN;

	/* Stop both engines, load them, then start them with one write so that
	 * two-colour patterns stay in phase.
	 *
	 * While both are stopped and forced off, check that the masks hold what
	 * is written. With the bus closed, reads return the last word read
	 * anywhere, which will not match these; a locked LED ignores writes. In
	 * case the bus needs a moment after opening, try again for a while. */
	for (ms = 0; ; ms++) {
		LED_CTRL = keep | LED_G_FORCE_OFF | LED_R_FORCE_OFF;
		LED_G_MASK = 0x5A5AC3C3u;
		LED_R_MASK = 0xA5A53C3Cu;
		writable = LED_G_MASK == 0x5A5AC3C3u && LED_R_MASK == 0xA5A53C3Cu;
		if (writable || ms == LED_WAIT_MS)
			break;
		msleep(1);
	}
	waited_ms = ms;

	LED_G_MASK = p->green;
	LED_G_DELAY = delay;
	LED_R_MASK = p->red;
	LED_R_DELAY = delay;
	LED_CTRL = keep
		| channel_bits(p->green, LED_G_BLINK | LED_G_ITER | LED_G_RUN, LED_G_FORCE_ON, LED_G_FORCE_OFF)
		| channel_bits(p->red, LED_R_BLINK | LED_R_ITER | LED_R_RUN, LED_R_FORCE_ON, LED_R_FORCE_OFF);
}

bool led_writable(unsigned *ms)
{
	if (ms)
		*ms = waited_ms;
	return writable;
}

void led_force(bool green, bool on)
{
	uint32_t engine = green ? LED_G_BLINK | LED_G_ITER | LED_G_RUN
	                        : LED_R_BLINK | LED_R_ITER | LED_R_RUN | LED_R_BIT13;
	uint32_t force_on = green ? LED_G_FORCE_ON : LED_R_FORCE_ON;
	uint32_t force_off = green ? LED_G_FORCE_OFF : LED_R_FORCE_OFF;

	bus_open();
	LED_CTRL = (LED_CTRL & ~(engine | force_on | force_off)) | (on ? force_on : force_off);
}

void led_off(void)
{
	led_pattern_t off = { 0, 0, LED_STEP_OS };
	led_apply(&off);
}

void led_read(led_regs_t *r)
{
	bus_open();
	r->ctrl = LED_CTRL;
	r->g_mask = LED_G_MASK;
	r->g_delay = LED_G_DELAY;
	r->r_mask = LED_R_MASK;
	r->r_delay = LED_R_DELAY;
}

void led_write(const led_regs_t *r)
{
	bus_open();
	LED_CTRL = (r->ctrl & ~(LED_G_RUN | LED_R_RUN)) | LED_G_FORCE_OFF | LED_R_FORCE_OFF;
	LED_G_MASK = r->g_mask;
	LED_G_DELAY = r->g_delay;
	LED_R_MASK = r->r_mask;
	LED_R_DELAY = r->r_delay;
	LED_CTRL = r->ctrl;
}
