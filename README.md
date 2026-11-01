# cx-led

An [Ndless](https://ndless.me) program that controls the status LED of the TI-Nspire CX. Choose a colour and a flash pattern; the LED keeps playing it after you leave the program, while you use the calculator and run other programs.

> **Status:** written from the published register documentation but not yet tried on a calculator. The CX II has not been tested; cx-led asks before it touches the LED there.

## How it keeps running in the background

The CX has a small LED controller at `0x90110B00` with one blink engine per colour. Each engine plays a 32-step on/off mask, over and over, at a set step length. cx-led loads the masks, starts the engines and exits. Nothing stays resident: the hardware does the blinking, so it costs no CPU time and cannot crash or slow down whatever runs next.

The pattern runs until something reprograms the controller: cx-led itself, another program, or the OS.

## Install

1. Install Ndless r2004 or later (r2022 is fine) on a TI-Nspire CX.
2. Get `cx-led.zip` from the latest successful build (the repository's **Actions** tab, newest run on `main`, **Artifacts**) or from a release.
3. Copy `cxled.tns` anywhere in My Documents. Copy any presets you want from `presets/`.
4. Run `cxled.tns` once. From then on, opening a `.led.tns` preset applies it.

## The menu

| Key | Action |
|---|---|
| ▲ ▼ (or 8 2) | Choose a row |
| ◄ ► (or 4 6) | Change the value |
| enter | Open *Edit pattern* or *LED info*, toggle *Run at startup* |
| menu | Open *LED info* |
| esc | Leave. The LED keeps its pattern and the setting is saved |

Every change goes to the LED at once. The large dot in the corner and the timeline at the bottom preview the pattern; their timing is approximate, the LED's is not.

- **Effect:** see the table below.
- **Colour:** for one-colour effects. Green, red and yellow (both LEDs at once). *Solid* also offers orange, amber, lime and dim green, red and yellow, mixed by switching the LEDs faster than the eye can follow.
- **Cycle:** the time for one pass of the 32 steps, from 0.25 s to 32 s.
- **Edit pattern:** draw your own pattern. The editor starts from the current effect's steps; a mixed *Solid* colour starts as plain green, red or yellow, and *Off* brings back your last custom pattern.
- **Run at startup:** apply this setting every time Ndless starts.
- **LED info:** the hardware, the Ndless revision and the live register values.

| Effect | Preset key | What it does |
|---|---|---|
| Off | `off` | LED off |
| Solid | `solid` | Steady light |
| Blink | `blink` | On for the first half of the cycle |
| Fast blink | `fast` | 4 flashes per cycle |
| Strobe | `strobe` | 16 short flashes per cycle |
| Beacon | `beacon` | One short flash per cycle |
| Double flash | `double` | Two flashes, then a pause |
| Triple flash | `triple` | Three flashes, then a pause |
| Heartbeat | `heartbeat` | A long beat and a short one |
| SOS | `sos` | ··· ––– ··· in Morse code |
| Alternate R/G | `alternate` | Red and green take turns |
| Traffic light | `traffic` | Green, yellow, red |
| Colour cycle | `cycle` | Green, yellow, red, off |
| Emergency | `emergency` | Red and green double flashes, taking turns |
| Custom | `custom` | Your own pattern |

### Edit pattern

The 32 steps are shown in two rows of 16.

| Key | Action |
|---|---|
| G R Y | Make the step green, red or yellow, then move on |
| 0 . space | Turn the step off, then move on |
| ◄ ► | Move between steps |
| ▲ ▼ | Change the step: off, green, red, yellow |
| del | Go back one step and turn it off |
| C | Clear every step |
| + − | Faster, slower |
| enter | Keep the pattern |
| esc | Cancel and go back to the previous setting |

## Presets

A preset is a text file whose name ends in `.led.tns`. Opening it in My Documents applies it with no screen in between and makes it the current setting, just as if you had chosen it in the menu. Each line is `key=value`; lines starting with `#` are comments.

```
# Red heartbeat, once a second
effect=heartbeat
colour=red
cycle=1
```

| Key | Values |
|---|---|
| `effect` | A key from the effect table |
| `colour` (or `color`) | `green`, `red`, `yellow`; with `solid` also `orange`, `amber`, `lime`, `dimgreen`, `dimred`, `dimyellow` |
| `cycle` | Seconds per cycle: `0.25`, `0.5`, `1`, `2`, `4`, `8`, `16` or `32` (`2s` works too) |
| `pattern` | Steps of the Custom effect: `G`, `R`, `Y` or `.` for off. 1, 2, 4, 8, 16 or 32 steps; shorter patterns repeat to fill 32 |

Keys a preset leaves out keep their current value. A `pattern` without an `effect` selects Custom. The `presets/` folder has examples. Write your own in any text editor and send it to the calculator like any other `.tns` file.

Opening presets works through a file association: the first time the menu opens, cx-led adds `ext.led=cxled` to `ndless.cfg.tns` (if a `.led` entry is already there, cx-led leaves it alone). If you rename `cxled.tns`, change that line to the new name. The name cannot contain spaces or `#`, which that file cannot hold; cx-led tells you when the menu opens if it does.

## Run at startup

Turning *Run at startup* on writes the current setting to `ndless/startup/cxled.led.tns`. Ndless opens the files in that folder when it starts, and cx-led applies the setting without showing anything. While the option is on, the startup setting follows the current one: leaving the menu or opening a preset updates it. Turn the option off, or delete the file, to stop. Holding esc while Ndless starts skips the startup folder.

## Files cx-led writes

All paths are inside My Documents.

| File | Contents |
|---|---|
| `ndless/cxled.cfg.tns` | The current setting, in preset format |
| `ndless/startup/cxled.led.tns` | The startup setting, only while *Run at startup* is on |
| `ndless/ndless.cfg.tns` | The `ext.led` association line, added once |

## Caveats

- **Untested hardware:** the program follows the register documentation on [Hackspire](https://hackspire.org/Memory-mapped_IO_ports_on_CX), but nobody has run it on a calculator yet.
- **CX II:** its LED controller is undocumented. cx-led asks before using it. It remembers a yes; after a no it asks again next time. At startup it does nothing on a CX II until you have said yes.
- **The OS uses the LED too:** if it reprograms the controller, your pattern stops until cx-led or a preset runs again.
- **Battery:** it is not known whether the LED keeps running while the calculator is off. If you are not sure, open the `off` preset before putting the calculator away. With *Run at startup* on, that also makes *Off* the startup setting.
- **Timing:** cycle times assume a 32768 Hz step clock. If the real clock differs, every cycle is off by the same factor.

## Using the driver in your own program

`src/led.h` and `src/led.c` do not depend on the rest of cx-led. Copy them into another Ndless project:

```c
#include "led.h"

/* Green flashes 4 times per pass; 32 steps of 512 ticks = 0.5 s per pass. */
led_pattern_t p = { .green = 0xF0F0F0F0, .red = 0, .step = 512 };
led_apply(&p);   /* returns at once, the LED keeps going */

led_off();
```

A mask is played from bit 31 down to bit 0, one bit per step. Red and green on together look yellow. `led_read()` and `led_write()` save and restore the raw registers, for a program that wants to put the LED back the way it found it.

### Register map

From Hackspire. Offsets are from `0x90110B00`.

| Offset | Register |
|---|---|
| `0x00` | Control |
| `0x04` | Green mask |
| `0x08` | Green step length, negated (the OS writes −2048) |
| `0x0C` | Red mask |
| `0x10` | Red step length, negated |

| Control bit | Meaning |
|---|---|
| 0 | Green follows its mask |
| 1 and 6 | Green steps through its mask (clear 6 before changing the mask or step) |
| 2 | Green forced off (wins over bit 4) |
| 3 | Red forced off (wins over bits 5 and 13) |
| 4 | Green forced on |
| 5 | Red forced on |
| 9 | Red follows its mask |
| 10 and 12 | Red steps through its mask (clear 12 before changing the mask or step) |
| 13 | Red forced on, or off when bit 4 is set (unclear; `led_apply()` clears it) |

## Building

With the [Ndless SDK](https://github.com/ndless-nspire/Ndless/wiki/Ndless-SDK:-C-and-assembly-development-introduction) on `PATH`:

```sh
make            # builds cxled.tns
make clean
```

Every push to `main` builds `cxled.tns` on GitHub Actions in a Debian 13 container with the Ndless nightly SDK package, and uploads it with the presets as the `cx-led` artifact. Pushing a tag such as `v1.0.0` also publishes a release with `cx-led.zip` and `cxled.tns`.

| Path | Contents |
|---|---|
| `src/main.c` | Start-up modes: menu, preset, startup |
| `src/led.c` | LED driver |
| `src/effects.c` | Effects, colours, cycle times |
| `src/settings.c` | Saved setting, presets, startup file |
| `src/ui.c` | Menu, pattern editor, LED info |
| `src/gfx.c` | Small drawing layer over `lcd_blit()` |
| `tools/psf2c.py` | Turns a PSF console font into `src/font8x16.h` |

## Licence

MIT, see [LICENSE](LICENSE). The font comes from the public-domain console-setup "Fixed" font.
