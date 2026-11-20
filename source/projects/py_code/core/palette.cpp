// SPDX-License-Identifier: MIT
/* palette.cpp -- see palette.h. The table is hex text, parsed once. */

#include "core/palette.h"

#include <cstdlib>
#include <sstream>

namespace pyc {

static const char *const kRoleNames[RoleCount] = {
	"bg",      "text",    "sel",     "caret",  "curline", "keyword", "builtin", "function",
	"class",   "decorator", "string", "escape", "number",  "self",    "comment", "bracket",
	"operator", "match",  "blink",   "bar",    "barhighlight", "error", "warning"};

struct ThemeSpec {
	const char *name;
	const char *aliases;
	const char *colors; /* 23 tokens `#rrggbb` or `#rrggbb@alpha` */
};

static const ThemeSpec kThemes[] = {
	{"onedark", "atom",
	 "#282c34 #abb2bf #3e4451 #528bff #2c313a #c678dd #56b6c2 #e06c75 #e5c07b #d19a66 #98c379 #98c379 "
	 "#d19a66 #e06c75 #5c6370 #7f848e #abb2bf #61afef@0.30 #61afef@0.40 #21252b #61afef #e06c75 #e5c07b"},
	{"catppuccin", "mocha catppuccinmocha",
	 "#1e1e2e #cdd6f4 #89b4fa@0.30 #f5e0dc #313244 #cba6f7 #94e2d5 #f5c2e7 #89b4fa #fab387 #a6e3a1 #f9e2af "
	 "#74c7ec #eba0ac #6c7086 #9399b2 #bac2de #b4befe@0.35 #fab387@0.40 #181825 #fab387 #f38ba8 #f9e2af"},
	{"dracula", "",
	 "#282a36 #f8f8f2 #44475a #f8f8f2 #323443 #ff79c6 #8be9fd #ffb86c #8be9fd #50fa7b #f1fa8c #f1fa8c "
	 "#bd93f9 #ffb86c #6272a4 #f8f8f2 #ff79c6 #8be9fd@0.25 #ff79c6@0.35 #21222c #bd93f9 #ff5555 #f1fa8c"},
	{"nord", "",
	 "#2e3440 #d8dee9 #434c5e #88c0d0 #3b4252 #81a1c1 #8fbcbb #b48ead #8fbcbb #a3be8c #a3be8c #ebcb8b "
	 "#b48ead #d08770 #616e88 #d8dee9 #81a1c1 #88c0d0@0.30 #ebcb8b@0.35 #242933 #88c0d0 #bf616a #ebcb8b"},
	{"gruvbox", "gruvboxdark",
	 "#282828 #ebdbb2 #504945 #fe8019 #32302f #fb4934 #8ec07c #d3869b #fabd2f #fe8019 #b8bb26 #b8bb26 "
	 "#d3869b #fe8019 #928374 #a89984 #ebdbb2 #83a598@0.35 #fe8019@0.35 #1d2021 #fe8019 #fb4934 #fabd2f"},
	{"monokai", "",
	 "#272822 #f8f8f2 #49483e #f8f8f2 #3e3d32 #f92672 #ae81ff #fd971f #66d9ef #a6e22e #e6db74 #e6db74 "
	 "#ae81ff #fd971f #75715e #f8f8f2 #f92672 #66d9ef@0.28 #f92672@0.35 #1e1f1c #a6e22e #f92672 #e6db74"},
	{"solarized", "solarizeddark solarised",
	 "#002b36 #839496 #073642 #93a1a1 #073642 #859900 #b58900 #6c71c4 #268bd2 #cb4b16 #2aa198 #2aa198 "
	 "#d33682 #cb4b16 #586e75 #93a1a1 #859900 #268bd2@0.35 #b58900@0.35 #00252e #268bd2 #dc322f #b58900"},
};

static Color parse_color(const std::string &tok) {
	Color c;
	const unsigned long v = std::strtoul(tok.substr(1, 6).c_str(), nullptr, 16);
	c.r = ((v >> 16) & 0xFF) / 255.0;
	c.g = ((v >> 8) & 0xFF) / 255.0;
	c.b = (v & 0xFF) / 255.0;
	const size_t at = tok.find('@');
	if (at != std::string::npos)
		c.a = std::strtod(tok.c_str() + at + 1, nullptr);
	return c;
}

static std::string normalize(const std::string &s) {
	std::string o;
	for (char c : s)
		if (c != ' ' && c != '-' && c != '_')
			o += (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
	return o;
}

static const std::vector<Theme> &themes() {
	static const std::vector<Theme> v = [] {
		std::vector<Theme> out;
		for (const ThemeSpec &spec : kThemes) {
			Theme t;
			t.name = spec.name;
			std::istringstream al(spec.aliases), cs(spec.colors);
			for (std::string a; al >> a;)
				t.aliases.push_back(a);
			int i = 0;
			for (std::string tok; cs >> tok && i < RoleCount;)
				t.colors[i++] = parse_color(tok);
			out.push_back(t);
		}
		return out;
	}();
	return v;
}

int theme_count() { return (int)themes().size(); }

const Theme &theme_at(int i) {
	const int n = theme_count();
	return themes()[(size_t)(i < 0 ? 0 : i >= n ? n - 1 : i)];
}

const Theme *theme_find(const std::string &name) {
	const std::string key = normalize(name);
	if (key.empty())
		return nullptr;
	for (const Theme &t : themes()) {
		if (t.name == key)
			return &t;
		for (const std::string &a : t.aliases)
			if (a == key)
				return &t;
	}
	return nullptr;
}

int role_count() { return RoleCount; }

const char *role_name(int i) { return i >= 0 && i < RoleCount ? kRoleNames[i] : ""; }

} // namespace pyc
