// SPDX-License-Identifier: MIT
/* py_theme.h -- named color presets for `@theme`.
 *
 * 21 roles, one per color attribute, in the order the object copies them.
 * Palette values are the published hex lists of each project (One Dark,
 * Catppuccin, Dracula, Nord, Gruvbox, Monokai, Solarized). */

#pragma once

namespace py_code {

/* Normalized color 0..1 with alpha (paint passes it to jgraphics_set_source_rgba). */
struct RGBA {
	double r, g, b, a;
};

/* One preset: name + aliases + the 21 roles, in this order. */
struct Theme {
	const char *name;    /* canonical name, the one `@theme` takes */
	const char *aliases; /* extra names, already normalized, space separated ("" if none) */
	RGBA bg, text, sel, caret, curline;
	RGBA keyword, builtin, function, klass, decorator, string, escape, number, self, comment, bracket, op;
	RGBA match, blink, bar, barhighlight;
};

/* How many presets there are (the first one, `onedark`, is the factory default). */
int theme_count();
const Theme *theme_at(int index);
/* The factory palette: One Dark (= `theme_at(0)`). */
const Theme *theme_default();
/* Preset by name, forgiving about case, spaces, '-' and '_' ("One-Dark" == "one dark"
 * == "ONEDARK") and accepting the aliases of each theme ("mocha" => catppuccin).
 * NULL if there is no such theme (or for NULL/empty). */
const Theme *theme_find(const char *name);

} // namespace py_code

/* --- One Dark = the factory palette -------------------------------------------
 * These strings are BOTH the RGBA defaults of the color attributes in `py_code.cpp`
 * (CLASS_ATTR_DEFAULT_SAVE_PAINT takes a string) AND what `py_theme.cpp` parses for
 * the `onedark` row, so "no @theme" and "@theme onedark" can never drift apart.
 * Format: "r g b a", 0..1. */
#define PY_THEME_ONEDARK_BG "0.156863 0.172549 0.203922 1."
#define PY_THEME_ONEDARK_TEXT "0.670588 0.698039 0.74902 1."
#define PY_THEME_ONEDARK_SEL "0.243137 0.266667 0.317647 1."
#define PY_THEME_ONEDARK_CARET "0.321569 0.545098 1. 1."
#define PY_THEME_ONEDARK_CURLINE "0.172549 0.192157 0.227451 1."
#define PY_THEME_ONEDARK_KEYWORD "0.776471 0.470588 0.866667 1."
#define PY_THEME_ONEDARK_BUILTIN "0.337255 0.713725 0.760784 1."
#define PY_THEME_ONEDARK_FUNCTION "0.878431 0.423529 0.458824 1."
#define PY_THEME_ONEDARK_CLASS "0.898039 0.752941 0.482353 1."
#define PY_THEME_ONEDARK_DECORATOR "0.819608 0.603922 0.4 1."
#define PY_THEME_ONEDARK_STRING "0.596078 0.764706 0.47451 1."
#define PY_THEME_ONEDARK_ESCAPE "0.596078 0.764706 0.47451 1."
#define PY_THEME_ONEDARK_NUMBER "0.819608 0.603922 0.4 1."
#define PY_THEME_ONEDARK_SELF "0.878431 0.423529 0.458824 1."
#define PY_THEME_ONEDARK_COMMENT "0.360784 0.388235 0.439216 1."
#define PY_THEME_ONEDARK_BRACKET "0.498039 0.517647 0.556863 1."
#define PY_THEME_ONEDARK_OPERATOR "0.670588 0.698039 0.74902 1."
#define PY_THEME_ONEDARK_MATCH "0.380392 0.686275 0.937255 0.3"
#define PY_THEME_ONEDARK_BLINK "0.380392 0.686275 0.937255 0.4"
#define PY_THEME_ONEDARK_BAR "0.129412 0.145098 0.168627 1."
#define PY_THEME_ONEDARK_BARHIGHLIGHT "0.380392 0.686275 0.937255 1."
