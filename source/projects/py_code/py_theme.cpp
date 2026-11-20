// SPDX-License-Identifier: MIT
/* py_theme.cpp -- see py_theme.h. */

#include "py_theme.h"

#include <cctype>
#include <cstdio>
#include <cstring>

namespace py_code {

namespace {

/* "r g b a" (3 values = alpha 1), clamped to 0..1. A malformed string leaves
 * black/opaque: it is a table bug, not something a user can reach. */
double clamp01(double v) {
	return (v < 0.) ? 0. : ((v > 1.) ? 1. : v);
}
RGBA rgba(const char *s) {
	double v[4] = {0., 0., 0., 1.};
	const int n = s ? sscanf(s, "%lf %lf %lf %lf", &v[0], &v[1], &v[2], &v[3]) : 0;
	RGBA c;
	c.r = clamp01(v[0]);
	c.g = clamp01(v[1]);
	c.b = clamp01(v[2]);
	c.a = clamp01(n >= 4 ? v[3] : 1.);
	return c;
}

/* Keeps only [a-z0-9], lowercased: "One-Dark" / "one dark" / "ONEDARK" => "onedark". */
void normalize(const char *in, char *out, int size) {
	int n = 0;
	if (size <= 0)
		return;
	out[0] = 0;
	if (!in)
		return;
	for (const char *p = in; *p && n < size - 1; ++p) {
		const unsigned char c = (unsigned char)*p;
		if (std::isalnum(c))
			out[n++] = (char)std::tolower(c);
	}
	out[n] = 0;
}

/* `key` is already normalized. */
bool same(const char *key, const char *candidate) {
	char buf[64];
	normalize(candidate, buf, (int)sizeof(buf));
	return buf[0] && std::strcmp(key, buf) == 0;
}

#define OA(m) rgba(PY_THEME_ONEDARK_##m)

/* One row per theme, one line per role (the hex it came from is in the comment, so a
 * value can be corrected without counting positions). One Dark does NOT repeat the
 * numbers: it parses the macros of py_theme.h, which are the same strings ext_main
 * uses as the attribute defaults. */
const Theme kThemes[] = {
	{"onedark", "atom",
	rgba(PY_THEME_ONEDARK_BG), /* #282c34  bg */
	rgba(PY_THEME_ONEDARK_TEXT), /* #abb2bf  text */
	rgba(PY_THEME_ONEDARK_SEL), /* #3e4451  sel */
	rgba(PY_THEME_ONEDARK_CARET), /* #528bff  caret */
	rgba(PY_THEME_ONEDARK_CURLINE), /* #2c313a  curline */
	rgba(PY_THEME_ONEDARK_KEYWORD), /* #c678dd  keyword */
	rgba(PY_THEME_ONEDARK_BUILTIN), /* #56b6c2  builtin */
	rgba(PY_THEME_ONEDARK_FUNCTION), /* #e06c75  function */
	rgba(PY_THEME_ONEDARK_CLASS), /* #e5c07b  klass */
	rgba(PY_THEME_ONEDARK_DECORATOR), /* #d19a66  decorator */
	rgba(PY_THEME_ONEDARK_STRING), /* #98c379  string */
	rgba(PY_THEME_ONEDARK_ESCAPE), /* #98c379  escape */
	rgba(PY_THEME_ONEDARK_NUMBER), /* #d19a66  number */
	rgba(PY_THEME_ONEDARK_SELF), /* #e06c75  self */
	rgba(PY_THEME_ONEDARK_COMMENT), /* #5c6370  comment */
	rgba(PY_THEME_ONEDARK_BRACKET), /* #7f848e  bracket */
	rgba(PY_THEME_ONEDARK_OPERATOR), /* #abb2bf  op */
	rgba(PY_THEME_ONEDARK_MATCH), /* #61afef @0.30  match */
	rgba(PY_THEME_ONEDARK_BLINK), /* #61afef @0.40  blink */
	rgba(PY_THEME_ONEDARK_BAR), /* #21252b  bar */
	rgba(PY_THEME_ONEDARK_BARHIGHLIGHT), /* #61afef  barhighlight */
	},
	{"catppuccin", "mocha catppuccinmocha",
	{0.117647, 0.117647, 0.180392, 1.}, /* #1e1e2e  bg */
	{0.803922, 0.839216, 0.956863, 1.}, /* #cdd6f4  text */
	{0.537255, 0.705882, 0.980392, 0.3}, /* #89b4fa @0.30  sel */
	{0.960784, 0.878431, 0.862745, 1.}, /* #f5e0dc  caret */
	{0.192157, 0.196078, 0.266667, 1.}, /* #313244  curline */
	{0.796078, 0.65098, 0.968627, 1.}, /* #cba6f7  keyword */
	{0.580392, 0.886275, 0.835294, 1.}, /* #94e2d5  builtin */
	{0.960784, 0.760784, 0.905882, 1.}, /* #f5c2e7  function */
	{0.537255, 0.705882, 0.980392, 1.}, /* #89b4fa  klass */
	{0.980392, 0.701961, 0.529412, 1.}, /* #fab387  decorator */
	{0.65098, 0.890196, 0.631373, 1.}, /* #a6e3a1  string */
	{0.976471, 0.886275, 0.686275, 1.}, /* #f9e2af  escape */
	{0.454902, 0.780392, 0.92549, 1.}, /* #74c7ec  number */
	{0.921569, 0.627451, 0.67451, 1.}, /* #eba0ac  self */
	{0.423529, 0.439216, 0.52549, 1.}, /* #6c7086  comment */
	{0.576471, 0.6, 0.698039, 1.}, /* #9399b2  bracket */
	{0.729412, 0.760784, 0.870588, 1.}, /* #bac2de  op */
	{0.705882, 0.745098, 0.996078, 0.35}, /* #b4befe @0.35  match */
	{0.980392, 0.701961, 0.529412, 0.4}, /* #fab387 @0.40  blink */
	{0.094118, 0.094118, 0.145098, 1.}, /* #181825  bar */
	{0.980392, 0.701961, 0.529412, 1.}, /* #fab387  barhighlight */
	},
	{"dracula", "",
	{0.156863, 0.164706, 0.211765, 1.}, /* #282a36  bg */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  text */
	{0.266667, 0.278431, 0.352941, 1.}, /* #44475a  sel */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  caret */
	{0.196078, 0.203922, 0.262745, 1.}, /* #323443  curline */
	{1., 0.47451, 0.776471, 1.}, /* #ff79c6  keyword */
	{0.545098, 0.913725, 0.992157, 1.}, /* #8be9fd  builtin */
	{1., 0.721569, 0.423529, 1.}, /* #ffb86c  function */
	{0.545098, 0.913725, 0.992157, 1.}, /* #8be9fd  klass */
	{0.313725, 0.980392, 0.482353, 1.}, /* #50fa7b  decorator */
	{0.945098, 0.980392, 0.54902, 1.}, /* #f1fa8c  string */
	{0.945098, 0.980392, 0.54902, 1.}, /* #f1fa8c  escape */
	{0.741176, 0.576471, 0.976471, 1.}, /* #bd93f9  number */
	{1., 0.721569, 0.423529, 1.}, /* #ffb86c  self */
	{0.384314, 0.447059, 0.643137, 1.}, /* #6272a4  comment */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  bracket */
	{1., 0.47451, 0.776471, 1.}, /* #ff79c6  op */
	{0.545098, 0.913725, 0.992157, 0.25}, /* #8be9fd @0.25  match */
	{1., 0.47451, 0.776471, 0.35}, /* #ff79c6 @0.35  blink */
	{0.129412, 0.133333, 0.172549, 1.}, /* #21222c  bar */
	{0.741176, 0.576471, 0.976471, 1.}, /* #bd93f9  barhighlight */
	},
	{"nord", "",
	{0.180392, 0.203922, 0.25098, 1.}, /* #2e3440  bg */
	{0.847059, 0.870588, 0.913725, 1.}, /* #d8dee9  text */
	{0.262745, 0.298039, 0.368627, 1.}, /* #434c5e  sel */
	{0.533333, 0.752941, 0.815686, 1.}, /* #88c0d0  caret */
	{0.231373, 0.258824, 0.321569, 1.}, /* #3b4252  curline */
	{0.505882, 0.631373, 0.756863, 1.}, /* #81a1c1  keyword */
	{0.560784, 0.737255, 0.733333, 1.}, /* #8fbcbb  builtin */
	{0.705882, 0.556863, 0.678431, 1.}, /* #b48ead  function */
	{0.560784, 0.737255, 0.733333, 1.}, /* #8fbcbb  klass */
	{0.639216, 0.745098, 0.54902, 1.}, /* #a3be8c  decorator */
	{0.639216, 0.745098, 0.54902, 1.}, /* #a3be8c  string */
	{0.921569, 0.796078, 0.545098, 1.}, /* #ebcb8b  escape */
	{0.705882, 0.556863, 0.678431, 1.}, /* #b48ead  number */
	{0.815686, 0.529412, 0.439216, 1.}, /* #d08770  self */
	{0.380392, 0.431373, 0.533333, 1.}, /* #616e88  comment */
	{0.847059, 0.870588, 0.913725, 1.}, /* #d8dee9  bracket */
	{0.505882, 0.631373, 0.756863, 1.}, /* #81a1c1  op */
	{0.533333, 0.752941, 0.815686, 0.3}, /* #88c0d0 @0.30  match */
	{0.921569, 0.796078, 0.545098, 0.35}, /* #ebcb8b @0.35  blink */
	{0.141176, 0.160784, 0.2, 1.}, /* #242933  bar */
	{0.533333, 0.752941, 0.815686, 1.}, /* #88c0d0  barhighlight */
	},
	{"gruvbox", "gruvboxdark",
	{0.156863, 0.156863, 0.156863, 1.}, /* #282828  bg */
	{0.921569, 0.858824, 0.698039, 1.}, /* #ebdbb2  text */
	{0.313725, 0.286275, 0.270588, 1.}, /* #504945  sel */
	{0.996078, 0.501961, 0.098039, 1.}, /* #fe8019  caret */
	{0.196078, 0.188235, 0.184314, 1.}, /* #32302f  curline */
	{0.984314, 0.286275, 0.203922, 1.}, /* #fb4934  keyword */
	{0.556863, 0.752941, 0.486275, 1.}, /* #8ec07c  builtin */
	{0.827451, 0.52549, 0.607843, 1.}, /* #d3869b  function */
	{0.980392, 0.741176, 0.184314, 1.}, /* #fabd2f  klass */
	{0.996078, 0.501961, 0.098039, 1.}, /* #fe8019  decorator */
	{0.721569, 0.733333, 0.14902, 1.}, /* #b8bb26  string */
	{0.721569, 0.733333, 0.14902, 1.}, /* #b8bb26  escape */
	{0.827451, 0.52549, 0.607843, 1.}, /* #d3869b  number */
	{0.996078, 0.501961, 0.098039, 1.}, /* #fe8019  self */
	{0.572549, 0.513725, 0.454902, 1.}, /* #928374  comment */
	{0.658824, 0.6, 0.517647, 1.}, /* #a89984  bracket */
	{0.921569, 0.858824, 0.698039, 1.}, /* #ebdbb2  op */
	{0.513725, 0.647059, 0.596078, 0.35}, /* #83a598 @0.35  match */
	{0.996078, 0.501961, 0.098039, 0.35}, /* #fe8019 @0.35  blink */
	{0.113725, 0.12549, 0.129412, 1.}, /* #1d2021  bar */
	{0.996078, 0.501961, 0.098039, 1.}, /* #fe8019  barhighlight */
	},
	{"monokai", "",
	{0.152941, 0.156863, 0.133333, 1.}, /* #272822  bg */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  text */
	{0.286275, 0.282353, 0.243137, 1.}, /* #49483e  sel */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  caret */
	{0.243137, 0.239216, 0.196078, 1.}, /* #3e3d32  curline */
	{0.976471, 0.14902, 0.447059, 1.}, /* #f92672  keyword */
	{0.682353, 0.505882, 1., 1.}, /* #ae81ff  builtin */
	{0.992157, 0.592157, 0.121569, 1.}, /* #fd971f  function */
	{0.4, 0.85098, 0.937255, 1.}, /* #66d9ef  klass */
	{0.65098, 0.886275, 0.180392, 1.}, /* #a6e22e  decorator */
	{0.901961, 0.858824, 0.454902, 1.}, /* #e6db74  string */
	{0.901961, 0.858824, 0.454902, 1.}, /* #e6db74  escape */
	{0.682353, 0.505882, 1., 1.}, /* #ae81ff  number */
	{0.992157, 0.592157, 0.121569, 1.}, /* #fd971f  self */
	{0.458824, 0.443137, 0.368627, 1.}, /* #75715e  comment */
	{0.972549, 0.972549, 0.94902, 1.}, /* #f8f8f2  bracket */
	{0.976471, 0.14902, 0.447059, 1.}, /* #f92672  op */
	{0.4, 0.85098, 0.937255, 0.28}, /* #66d9ef @0.28  match */
	{0.976471, 0.14902, 0.447059, 0.35}, /* #f92672 @0.35  blink */
	{0.117647, 0.121569, 0.109804, 1.}, /* #1e1f1c  bar */
	{0.65098, 0.886275, 0.180392, 1.}, /* #a6e22e  barhighlight */
	},
	{"solarized", "solarizeddark solarised",
	{0., 0.168627, 0.211765, 1.}, /* #002b36  bg */
	{0.513725, 0.580392, 0.588235, 1.}, /* #839496  text */
	{0.027451, 0.211765, 0.258824, 1.}, /* #073642  sel */
	{0.576471, 0.631373, 0.631373, 1.}, /* #93a1a1  caret */
	{0.027451, 0.211765, 0.258824, 1.}, /* #073642  curline */
	{0.521569, 0.6, 0., 1.}, /* #859900  keyword */
	{0.709804, 0.537255, 0., 1.}, /* #b58900  builtin */
	{0.423529, 0.443137, 0.768627, 1.}, /* #6c71c4  function */
	{0.14902, 0.545098, 0.823529, 1.}, /* #268bd2  klass */
	{0.796078, 0.294118, 0.086275, 1.}, /* #cb4b16  decorator */
	{0.164706, 0.631373, 0.596078, 1.}, /* #2aa198  string */
	{0.164706, 0.631373, 0.596078, 1.}, /* #2aa198  escape */
	{0.827451, 0.211765, 0.509804, 1.}, /* #d33682  number */
	{0.796078, 0.294118, 0.086275, 1.}, /* #cb4b16  self */
	{0.345098, 0.431373, 0.458824, 1.}, /* #586e75  comment */
	{0.576471, 0.631373, 0.631373, 1.}, /* #93a1a1  bracket */
	{0.521569, 0.6, 0., 1.}, /* #859900  op */
	{0.14902, 0.545098, 0.823529, 0.35}, /* #268bd2 @0.35  match */
	{0.709804, 0.537255, 0., 0.35}, /* #b58900 @0.35  blink */
	{0., 0.145098, 0.180392, 1.}, /* #00252e  bar */
	{0.14902, 0.545098, 0.823529, 1.}, /* #268bd2  barhighlight */
	},
};

#undef OA

} // namespace

int theme_count() {
	return (int)(sizeof(kThemes) / sizeof(kThemes[0]));
}

const Theme *theme_at(int index) {
	if (index < 0 || index >= theme_count())
		return NULL;
	return &kThemes[index];
}

const Theme *theme_default() {
	return &kThemes[0]; /* onedark: the factory palette */
}

const Theme *theme_find(const char *name) {
	char key[64];
	normalize(name, key, (int)sizeof(key));
	if (!key[0])
		return NULL;
	for (int i = 0; i < theme_count(); ++i) {
		if (same(key, kThemes[i].name))
			return &kThemes[i];
		const char *a = kThemes[i].aliases;
		while (a && *a) {
			while (*a == ' ')
				++a;
			char tok[64];
			int j = 0;
			while (*a && *a != ' ' && j < (int)sizeof(tok) - 1)
				tok[j++] = *a++;
			tok[j] = 0;
			if (j > 0 && same(key, tok))
				return &kThemes[i];
		}
	}
	return NULL;
}

} // namespace py_code
