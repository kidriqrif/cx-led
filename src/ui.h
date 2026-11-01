/* ui.h - the menu, the pattern editor and the register view. */
#ifndef CXLED_UI_H
#define CXLED_UI_H

#include <stdbool.h>

#include "effects.h"

/* Shows the menu until Esc, programming the LED as changes are made.
 * *startup is the "Run at startup" switch. Returns false if the screen
 * could not be set up. */
bool ui_run(selection_t *s, bool *startup);

#endif
