#include <libndls.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "settings.h"

#define PRESET_EXT "led"

static const char *doc_path(char *buf, size_t size, const char *rel)
{
	snprintf(buf, size, "%s%s", get_documents_dir(), rel);
	return buf;
}

static const char *config_path(void)
{
	static char buf[128];
	return doc_path(buf, sizeof(buf), "ndless/cxled.cfg.tns");
}

static const char *startup_path(void)
{
	static char buf[128];
	return doc_path(buf, sizeof(buf), "ndless/startup/cxled.led.tns");
}

static void make_dir(const char *rel)
{
	char buf[128];
	mkdir(doc_path(buf, sizeof(buf), rel), 0755); /* fails if it exists */
}

static char message[80];

static const char *bad_value(const char *key, const char *value)
{
	snprintf(message, sizeof(message), "Unknown %s \"%.24s\".", key, value);
	return message;
}

/* 1 to 32 steps, repeated to fill the 32 the LED plays. */
static const char *parse_pattern(const char *v, uint32_t *green, uint32_t *red)
{
	size_t n = strlen(v);
	uint32_t g = 0, r = 0;

	if (n == 0 || n > LED_STEPS || LED_STEPS % n)
		return "A pattern needs 1, 2, 4, 8, 16 or 32 steps.";
	for (unsigned i = 0; i < LED_STEPS; i++) {
		uint32_t bit = 0x80000000u >> i;
		switch (v[i % n]) {
		case 'G': case 'g': g |= bit; break;
		case 'R': case 'r': r |= bit; break;
		case 'Y': case 'y': g |= bit; r |= bit; break;
		case '.': case '-': case '_': case '0': break;
		default: return "Pattern steps must be G, R, Y or a dot.";
		}
	}
	*green = g;
	*red = r;
	return NULL;
}

static const char *settings_read(const char *path, selection_t *s, bool *cx2_ok)
{
	const char *err = NULL;
	selection_t t = *s;
	int found = 0;
	char *v;
	FILE *f;

	/* cfg_open_file() fails silently, so check first. */
	if (!(f = fopen(path, "rb")))
		return "Cannot open the file.";
	fclose(f);

	cfg_open_file(path);
	if ((v = cfg_get("effect"))) {
		int i = effect_find(v);
		found++;
		if (i < 0)
			err = bad_value("effect", v);
		else
			t.effect = i;
	}
	if ((v = cfg_get("colour")) || (v = cfg_get("color"))) {
		int i = colour_find(v);
		found++;
		if (i < 0)
			err = bad_value("colour", v);
		else
			t.colour = i;
	}
	if ((v = cfg_get("cycle"))) {
		size_t n = strlen(v);
		int i;
		if (n > 1 && v[n - 1] == 's') /* allow "2s" */
			v[n - 1] = '\0';
		i = speed_find(v);
		found++;
		if (i < 0)
			err = bad_value("cycle", v);
		else
			t.speed = i;
	}
	if ((v = cfg_get("pattern"))) {
		const char *e = parse_pattern(v, &t.custom_green, &t.custom_red);
		found++;
		if (e)
			err = e;
		else if (!cfg_get("effect"))
			t.effect = effect_find("custom");
	}
	if (cx2_ok && (v = cfg_get("cx2")))
		*cx2_ok = !strcmp(v, "ok");
	cfg_close();

	if (!err && !found)
		err = "The file has no cx-led settings.";
	if (err)
		return err;
	selection_fix(&t);
	*s = t;
	return NULL;
}

static bool settings_write(const char *path, const selection_t *s, bool cx2_ok)
{
	char pattern[LED_STEPS + 1];
	FILE *f;
	bool ok;

	for (unsigned i = 0; i < LED_STEPS; i++) {
		bool g = led_bit(s->custom_green, i), r = led_bit(s->custom_red, i);
		pattern[i] = g && r ? 'Y' : g ? 'G' : r ? 'R' : '.';
	}
	pattern[LED_STEPS] = '\0';

	if (!(f = fopen(path, "wb")))
		return false;
	fprintf(f, "# cx-led settings\neffect=%s\ncolour=%s\ncycle=%s\npattern=%s\n",
	        effects[s->effect].key, colours[s->colour].key, speeds[s->speed].key, pattern);
	if (cx2_ok)
		fputs("cx2=ok\n", f);
	ok = !ferror(f);
	return fclose(f) == 0 && ok;
}

const char *config_load(selection_t *s, bool *cx2_ok)
{
	return settings_read(config_path(), s, cx2_ok);
}

bool config_save(const selection_t *s, bool cx2_ok)
{
	make_dir("ndless");
	return settings_write(config_path(), s, cx2_ok);
}

const char *preset_load(const char *path, selection_t *s)
{
	return settings_read(path, s, NULL);
}

bool startup_enabled(void)
{
	struct stat st;
	return stat(startup_path(), &st) == 0;
}

bool startup_set(bool on, const selection_t *s)
{
	if (!on)
		return unlink(startup_path()) == 0;
	make_dir("ndless");
	make_dir("ndless/startup");
	return settings_write(startup_path(), s, false);
}

bool register_presets(const char *argv0)
{
	char name[256] = "cxled";
	const char *base = argv0 ? strrchr(argv0, '/') : NULL;
	size_t n;

	base = base ? base + 1 : argv0;
	if (base && (n = strlen(base)) > 4 && !strcmp(base + n - 4, ".tns")) {
		/* in ndless.cfg.tns a value ends at a space and '#' starts a comment */
		if (n - 4 >= sizeof(name) || strcspn(base, " #") < n - 4)
			return false;
		memcpy(name, base, n - 4);
		name[n - 4] = '\0';
	}
	cfg_register_fileext(PRESET_EXT, name);
	return true;
}
