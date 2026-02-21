# zmk-caps-word-ru

Custom **Caps Word** behavior for **ZMK v0.3.x** that works better with Russian typing and avoids accidental `Ctrl+Shift+…`.

This module provides `&caps_word_ru` — a drop-in alternative to ZMK’s built-in `&caps_word`, implemented as an **out-of-tree ZMK/Zephyr module** (no ZMK fork required).

## Why?

ZMK’s built-in `&caps_word` is great, but it has two practical limitations for RU users:

1. **Only `A..Z` are treated as “letters”** for auto-shift.
   - On a Russian OS layout, letters like **`ХЪЖЭБЮ`** are produced by keys that are **punctuation in US layout** (`[ ] ; ' , .`), so `&caps_word` does not auto-shift them.
2. **Modifiers do not deactivate Caps Word**.
   - With Caps Word active, pressing `Ctrl+Z` may become `Ctrl+Shift+Z` (undo → redo).

`caps_word_ru` solves both.

## Differences vs `&caps_word`

### ✅ RU-friendly “letters”
In addition to `A..Z`, `caps_word_ru` treats these keycodes as alphabetic targets:

- `[` `]` `;` `'` `,` `.`

On a Russian OS layout these correspond to **`ХЪЖЭБЮ`** (ЙЦУКЕН), so Caps Word will auto-shift them too.

> Note: On an English OS layout, this means those punctuation keys will also get Shift while Caps Word is active (e.g. `[` → `{`). This is a trade-off because the firmware cannot know the OS layout.

### ✅ “Only Shift is allowed”
While `caps_word_ru` is active:

- **Shift is allowed** (held explicitly, or applied implicitly).
- Any involvement of **Ctrl / Alt / GUI** will **immediately deactivate** `caps_word_ru`.

This includes:
- `&kp LC(X)` / `&kp LA(X)` style bindings (mods are implicit on the key event)
- Manual sequences like holding Ctrl and pressing a key (`explicit mods`)

### ✅ No ZMK fork
This is an **out-of-tree module** you add via `west.yml`.

## Compatibility

- Designed for **ZMK v0.3.0** and expected to work across **v0.3.x**.
- Tested as a behavior module compiled into `app` target.

If you use a significantly newer ZMK version, API changes may require small updates.

## Installation

### 1) Add module to your `config/west.yml`

In your `zmk-config` repository, edit `config/west.yml` and add this project.

Example:

```yaml
manifest:
  remotes:
    - name: zmkfirmware
      url-base: https://github.com/zmkfirmware
    - name: idefant
      url-base: https://github.com/idefant

  projects:
    - name: zmk
      remote: zmkfirmware
      revision: v0.3.0
      import: app/west.yml

    - name: zmk-caps-word-ru
      remote: idefant
      revision: main

  self:
    path: config
```

Commit and push.

### 2) Include the DTSI in your keymap

In your `<keyboard>.keymap` (or whichever `.keymap` file you use), add:

```c
#include <behaviors/caps_word_ru.dtsi>
```

This declares a default `&caps_word_ru` instance in the `/behaviors` node.

### 3) Use `&caps_word_ru` in bindings

Example:

```dts
/ {
  keymap {
    compatible = "zmk,keymap";

    default_layer {
      bindings = <
        &caps_word_ru  &kp A  &kp B
      >;
    };
  };
};
```

### 4) (Optional) Customize properties

You can override the defaults in your keymap:

```dts
&caps_word_ru {
  mods = <(MOD_LSFT)>;

  /* Keys that do not deactivate caps word
   * (besides alphas/numerics which always continue).
   */
  continue-list = <UNDERSCORE BACKSPACE DELETE>;
};
```

## Defaults

The default instance from `dts/behaviors/caps_word_ru.dtsi` uses:

* `mods = MOD_LSFT`
* `continue-list = UNDERSCORE BACKSPACE DELETE`

## How it works (high level)

* Listens to `zmk_keycode_state_changed` events.
* When active, it:

  1. Cancels itself if `Ctrl/Alt/GUI` are involved (implicit or explicit)
  2. Cancels itself if a modifier key is pressed (except Shift)
  3. Adds `Shift` to “alphabetic” keys (A–Z plus `[ ] ; ' , .`)
  4. Deactivates on keys not considered “continuations”

## Troubleshooting

### Build error: `zmk/behavior.h: No such file or directory`

Your module must add sources to the `app` target, not build as a standalone Zephyr library.

This module’s `CMakeLists.txt` uses:

* `target_sources(app PRIVATE …)`

If you copied code into another module, make sure you do the same.

### Keycodes not found (`HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET...`)

ZMK v0.3.x uses long HID names like:

* `HID_USAGE_KEY_KEYBOARD_LEFT_BRACKET_AND_LEFT_BRACE`
* `HID_USAGE_KEY_KEYBOARD_APOSTROPHE_AND_QUOTE`

If you see “undeclared” errors, check the exact symbol names in your ZMK version.

## License

MIT (same spirit as ZMK). See `LICENSE` if present in your fork/repo.

## Credits

* Based on ZMK’s upstream `behavior_caps_word.c` (v0.3.0), adapted for RU typing and modifier-safe behavior.
