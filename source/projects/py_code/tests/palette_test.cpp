// SPDX-License-Identifier: MIT
/* palette_test.cpp -- themes: names, aliases, role order, parsed colors. */

#include "test.h"

#include "core/palette.h"

#include <cmath>

using namespace pyc;
using std::string;

static bool same(const Color &c, unsigned rgb, double a = 1) {
	auto eq = [](double x, double y) { return std::fabs(x - y) < 1e-9; };
	return eq(c.r, ((rgb >> 16) & 0xFF) / 255.0) && eq(c.g, ((rgb >> 8) & 0xFF) / 255.0) &&
		   eq(c.b, (rgb & 0xFF) / 255.0) && eq(c.a, a);
}

TEST(palette_roles) {
	const char *names[] = {"bg",      "text",    "sel",     "caret",  "curline", "keyword",
						   "builtin", "function", "class",  "decorator", "string", "escape",
						   "number",  "self",    "comment", "bracket", "operator", "match",
						   "blink",   "bar",     "barhighlight", "error", "warning"};
	CHECK_EQ(role_count(), 23);
	for (int i = 0; i < 23; ++i)
		CHECK_EQ(string(role_name(i)), string(names[i]));
	CHECK_EQ(string(role_name(23)), string(""));
	CHECK_EQ(string(role_name(-1)), string(""));
	CHECK_EQ((int)RoleWarning, 22);
}

TEST(palette_themes) {
	const char *names[] = {"onedark", "catppuccin", "dracula", "nord", "gruvbox", "monokai", "solarized"};
	CHECK_EQ(theme_count(), 7);
	for (int i = 0; i < 7; ++i)
		CHECK_EQ(theme_at(i).name, string(names[i]));
	CHECK_EQ(theme_at(-1).name, string("onedark")); /* clamped */
	CHECK_EQ(theme_at(99).name, string("solarized"));
}

TEST(palette_find) {
	struct {
		const char *query;
		const char *want; /* "" = not found */
	} cases[] = {
		{"onedark", "onedark"},       {"OneDark", "onedark"},       {"one-dark", "onedark"},
		{"one_dark", "onedark"},      {"One Dark", "onedark"},      {"atom", "onedark"},
		{"catppuccin", "catppuccin"}, {"mocha", "catppuccin"},      {"Catppuccin Mocha", "catppuccin"},
		{"catppuccin-mocha", "catppuccin"}, {"Dracula", "dracula"}, {"NORD", "nord"},
		{"gruvbox", "gruvbox"},       {"gruvbox-dark", "gruvbox"},  {"monokai", "monokai"},
		{"solarized", "solarized"},   {"Solarized Dark", "solarized"}, {"solarised", "solarized"},
		{"", ""},                     {"zenburn", ""},              {"dark", ""}, {"one dark pro", ""},
	};
	for (auto &c : cases) {
		const Theme *t = theme_find(c.query);
		CHECK_EQ(string(t ? t->name : ""), string(c.want));
	}
}

TEST(palette_colors) {
	const Theme &od = *theme_find("onedark");
	CHECK(same(od[RoleBg], 0x282c34));
	CHECK(same(od[RoleText], 0xabb2bf));
	CHECK(same(od[RoleKeyword], 0xc678dd));
	CHECK(same(od[RoleComment], 0x5c6370));
	CHECK(same(od[RoleMatch], 0x61afef, 0.30));
	CHECK(same(od[RoleBlink], 0x61afef, 0.40));
	CHECK(same(od[RoleBar], 0x21252b));
	CHECK(same(od[RoleBarHighlight], 0x61afef));
	const Theme &cat = *theme_find("catppuccin");
	CHECK(same(cat[RoleSel], 0x89b4fa, 0.30));
	CHECK(same(cat[RoleMatch], 0xb4befe, 0.35));
	CHECK(same(theme_find("monokai")->colors[RoleMatch], 0x66d9ef, 0.28));
	CHECK(same(theme_find("solarized")->colors[RoleBar], 0x00252e));
	CHECK(same(theme_find("nord")->colors[RoleOperator], 0x81a1c1));
}

TEST(palette_error_warning) {
	struct {
		const char *name;
		unsigned error, warning;
	} cases[] = {
		{"onedark", 0xe06c75, 0xe5c07b}, {"catppuccin", 0xf38ba8, 0xf9e2af}, {"dracula", 0xff5555, 0xf1fa8c},
		{"nord", 0xbf616a, 0xebcb8b},    {"gruvbox", 0xfb4934, 0xfabd2f},    {"monokai", 0xf92672, 0xe6db74},
		{"solarized", 0xdc322f, 0xb58900},
	};
	for (auto &c : cases) {
		const Theme &t = *theme_find(c.name);
		CHECK(same(t[RoleError], c.error));
		CHECK(same(t[RoleWarning], c.warning));
	}
}

TEST(palette_all_parsed) {
	/* No table color is black or out of range: a short row would leave defaults. */
	for (int i = 0; i < theme_count(); ++i)
		for (int r = 0; r < role_count(); ++r) {
			const Color &c = theme_at(i)[r];
			CHECK(c.r + c.g + c.b > 0);
			CHECK(c.r <= 1 && c.g <= 1 && c.b <= 1 && c.a > 0 && c.a <= 1);
		}
}
