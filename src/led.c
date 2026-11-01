#include <libndls.h>

#include "led.h"

bool led_supported(void)
{
	/* The CX II has a new ASIC and 0x90110B00 is not documented there. */
	return !is_cx2;
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
	uint32_t keep = LED_CTRL & ~LED_CTRL_KNOWN;

	/* Stop both engines, load them, then start them with one write so that
	 * two-colour patterns stay in phase. */
	LED_CTRL = keep | LED_G_FORCE_OFF | LED_R_FORCE_OFF;
	LED_G_MASK = p->green;
	LED_G_DELAY = delay;
	LED_R_MASK = p->red;
	LED_R_DELAY = delay;
	LED_CTRL = keep
		| channel_bits(p->green, LED_G_BLINK | LED_G_ITER | LED_G_RUN, LED_G_FORCE_ON, LED_G_FORCE_OFF)
		| channel_bits(p->red, LED_R_BLINK | LED_R_ITER | LED_R_RUN, LED_R_FORCE_ON, LED_R_FORCE_OFF);
}

void led_off(void)
{
	led_pattern_t off = { 0, 0, LED_STEP_OS };
	led_apply(&off);
}

void led_read(led_regs_t *r)
{
	r->ctrl = LED_CTRL;
	r->g_mask = LED_G_MASK;
	r->g_delay = LED_G_DELAY;
	r->r_mask = LED_R_MASK;
	r->r_delay = LED_R_DELAY;
}

void led_write(const led_regs_t *r)
{
	LED_CTRL = (r->ctrl & ~(LED_G_RUN | LED_R_RUN)) | LED_G_FORCE_OFF | LED_R_FORCE_OFF;
	LED_G_MASK = r->g_mask;
	LED_G_DELAY = r->g_delay;
	LED_R_MASK = r->r_mask;
	LED_R_DELAY = r->r_delay;
	LED_CTRL = r->ctrl;
}
