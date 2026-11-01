/* gfx.h - tiny RGB565 framebuffer renderer on top of lcd_blit(). */
#ifndef CXLED_GFX_H
#define CXLED_GFX_H

#include <stdbool.h>
#include <stdint.h>

#define GFX_W 320
#define GFX_H 240

#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

/* UI symbols in the font (control codes) */
#define SYM_LEFT  "\x01"
#define SYM_RIGHT "\x02"
#define SYM_UP    "\x03"
#define SYM_DOWN  "\x04"
#define SYM_DOT   "\x05"

bool gfx_init(void);
void gfx_quit(void);
void gfx_present(void);

void gfx_clear(uint16_t c);
void gfx_rect(int x, int y, int w, int h, uint16_t c);
void gfx_frame(int x, int y, int w, int h, uint16_t c);
void gfx_disc(int cx, int cy, int r, uint16_t c);
uint16_t gfx_mix(uint16_t a, uint16_t b, unsigned t); /* t = 0..256 from a to b */

/* Draws s with its top-left corner at (x, y), scaled by scale; returns the
 * x after the last glyph. */
int gfx_text(int x, int y, const char *s, uint16_t c, int scale);
int gfx_text_width(const char *s, int scale);

#endif
