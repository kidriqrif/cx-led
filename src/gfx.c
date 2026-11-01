#include <libndls.h>
#include <stdlib.h>
#include <string.h>

#include "font8x16.h"
#include "gfx.h"

static uint16_t *fb;

bool gfx_init(void)
{
	/* Heap, not .bss: keeps the program image small. */
	fb = malloc(GFX_W * GFX_H * sizeof(*fb));
	if (!fb)
		return false;
	if (!lcd_init(SCR_320x240_565)) {
		free(fb);
		fb = NULL;
		return false;
	}
	return true;
}

void gfx_quit(void)
{
	if (!fb)
		return;
	lcd_init(SCR_TYPE_INVALID);
	free(fb);
	fb = NULL;
}

void gfx_present(void)
{
	lcd_blit(fb, SCR_320x240_565);
}

void gfx_clear(uint16_t c)
{
	for (int i = 0; i < GFX_W * GFX_H; i++)
		fb[i] = c;
}

void gfx_rect(int x, int y, int w, int h, uint16_t c)
{
	if (x < 0) { w += x; x = 0; }
	if (y < 0) { h += y; y = 0; }
	if (x + w > GFX_W) w = GFX_W - x;
	if (y + h > GFX_H) h = GFX_H - y;
	if (w <= 0 || h <= 0)
		return;
	for (int row = y; row < y + h; row++) {
		uint16_t *p = fb + row * GFX_W + x;
		for (int i = 0; i < w; i++)
			p[i] = c;
	}
}

void gfx_frame(int x, int y, int w, int h, uint16_t c)
{
	gfx_rect(x, y, w, 1, c);
	gfx_rect(x, y + h - 1, w, 1, c);
	gfx_rect(x, y, 1, h, c);
	gfx_rect(x + w - 1, y, 1, h, c);
}

void gfx_disc(int cx, int cy, int r, uint16_t c)
{
	for (int dy = -r; dy <= r; dy++) {
		int dx = 0;
		while ((dx + 1) * (dx + 1) + dy * dy <= r * r)
			dx++;
		gfx_rect(cx - dx, cy + dy, 2 * dx + 1, 1, c);
	}
}

uint16_t gfx_mix(uint16_t a, uint16_t b, unsigned t)
{
	unsigned ra = a >> 11, ga = (a >> 5) & 63, ba = a & 31;
	unsigned rb = b >> 11, gb = (b >> 5) & 63, bb = b & 31;
	unsigned r = (ra * (256 - t) + rb * t) >> 8;
	unsigned g = (ga * (256 - t) + gb * t) >> 8;
	unsigned bl = (ba * (256 - t) + bb * t) >> 8;
	return (uint16_t)((r << 11) | (g << 5) | bl);
}

int gfx_text(int x, int y, const char *s, uint16_t c, int scale)
{
	for (; *s; s++, x += FONT_W * scale) {
		unsigned char ch = (unsigned char)*s;
		const uint8_t *glyph = font8x16[ch < FONT_COUNT ? ch : '?'];
		for (int row = 0; row < FONT_H; row++)
			for (int col = 0; col < FONT_W; col++)
				if (glyph[row] & (0x80 >> col))
					gfx_rect(x + col * scale, y + row * scale, scale, scale, c);
	}
	return x;
}

int gfx_text_width(const char *s, int scale)
{
	return (int)strlen(s) * FONT_W * scale;
}
