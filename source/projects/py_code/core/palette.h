// SPDX-License-Identifier: MIT
/* palette.h -- color themes: 23 roles per theme, RGBA doubles 0..1. */

#pragma once

#include <string>
#include <vector>

namespace pyc {

enum Role {
	RoleBg,
	RoleText,
	RoleSel,
	RoleCaret,
	RoleCurline,
	RoleKeyword,
	RoleBuiltin,
	RoleFunction,
	RoleClass,
	RoleDecorator,
	RoleString,
	RoleEscape,
	RoleNumber,
	RoleSelf,
	RoleComment,
	RoleBracket,
	RoleOperator,
	RoleMatch,
	RoleBlink,
	RoleBar,
	RoleBarHighlight,
	RoleError,
	RoleWarning,
	RoleCount
};

struct Color {
	double r = 0, g = 0, b = 0, a = 1;
};

struct Theme {
	std::string name;
	std::vector<std::string> aliases;
	Color colors[RoleCount];
	const Color &operator[](int role) const { return colors[role]; }
};

int theme_count();
const Theme &theme_at(int i);                 /* `i` clamped */
const Theme *theme_find(const std::string &name); /* case, ' ', '-', '_' ignored; null if unknown */
int role_count();
const char *role_name(int i); /* "bg", "text", ...; "" out of range */

} // namespace pyc
