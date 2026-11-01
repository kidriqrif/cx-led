/*
 * led.h - driver for the TI-Nspire CX status LED (the one above the screen).
 *
 * The LED controller at 0x90110B00 has a small blink engine for each colour
 * channel: a 32-bit on/off mask that the hardware plays back from bit 31 to
 * bit 0, one bit per step, over and over. Once programmed it needs no CPU
 * time at all, so a pattern keeps running after the program has exited.
 * Red and green on together look yellow.
 *
 * Register map: https://hackspire.org/Memory-mapped_IO_ports_on_CX
 *
 * These files are self-contained: copy led.h and led.c into another Ndless
 * project to drive the LED from it.
 */
#ifndef CXLED_LED_H
#define CXLED_LED_H

#include <stdbool.h>
#include <stdint.h>

#define LED_BASE        0x90110B00u
#define LED_REG(off)    (*(volatile uint32_t *)(LED_BASE + (off)))
#define LED_CTRL        LED_REG(0x00)
#define LED_G_MASK      LED_REG(0x04)
#define LED_G_DELAY     LED_REG(0x08)   /* step length, written negated */
#define LED_R_MASK      LED_REG(0x0C)
#define LED_R_DELAY     LED_REG(0x10)

/* LED_CTRL bits */
#define LED_G_BLINK     (1u << 0)   /* drive green from its mask */
#define LED_G_ITER      (1u << 1)   /* step through the green mask (needs LED_G_RUN) */
#define LED_G_FORCE_OFF (1u << 2)   /* wins over LED_G_FORCE_ON */
#define LED_R_FORCE_OFF (1u << 3)   /* wins over LED_R_FORCE_ON */
#define LED_G_FORCE_ON  (1u << 4)
#define LED_R_FORCE_ON  (1u << 5)
#define LED_G_RUN       (1u << 6)   /* clear before changing the green mask or delay */
#define LED_R_BLINK     (1u << 9)
#define LED_R_ITER      (1u << 10)
#define LED_R_RUN       (1u << 12)  /* clear before changing the red mask or delay */
#define LED_R_BIT13     (1u << 13)  /* forces red on (off if bit 4 is set); led_apply() clears it */
#define LED_CTRL_KNOWN  0x367Fu     /* bits 0-6, 9, 10, 12, 13 */

#define LED_STEPS       32
#define LED_HZ          32768u      /* assumed step clock */
#define LED_STEP_OS     2048u       /* step length the OS uses: 62.5 ms */
#define LED_STEP_MIN    8u
#define LED_STEP_MAX    32768u

typedef struct {
	uint32_t green;  /* on/off mask, bit 31 is played first */
	uint32_t red;
	uint32_t step;   /* step length in LED clock ticks */
} led_pattern_t;

typedef struct {
	uint32_t ctrl, g_mask, g_delay, r_mask, r_delay;
} led_regs_t;

/* False on the CX II: its ASIC is new and nobody has checked that the LED
 * controller works the same way there. Ask the user before going ahead. */
bool led_supported(void);

/* Program both channels and start them in step. A mask of all ones or all
 * zeros uses the force bits instead of the blink engine. */
void led_apply(const led_pattern_t *p);
void led_off(void);

void led_read(led_regs_t *r);
void led_write(const led_regs_t *r);

/* Whether a channel playing this mask is lit at step 0..31. */
static inline bool led_bit(uint32_t mask, unsigned step)
{
	return (mask >> (31 - (step & 31))) & 1;
}

#endif
