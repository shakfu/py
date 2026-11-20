// SPDX-License-Identifier: MIT
/* paths_test.cpp -- posix_path, expand_home, unquote, python_string, drop_text. */

#include "test.h"

#include "core/paths.h"

using namespace pyc;
using std::string;

TEST(paths_posix_path) {
	struct {
		const char *in, *want;
	} cases[] = {
		{"Macintosh HD:/Users/a/x.wav", "/Users/a/x.wav"},
		{"Data:/", "/"},
		{"/Users/a:/b", "/Users/a:/b"}, /* already POSIX */
		{"relative/x", "relative/x"},
		{"C:foo", "C:foo"},
		{"a:b:/c", "/c"},
		{"", ""},
	};
	for (auto &c : cases)
		CHECK_EQ(posix_path(c.in), string(c.want));
}

TEST(paths_expand_home) {
	struct {
		const char *home, *in, *want;
	} cases[] = {
		{"/Users/a", "~", "/Users/a"},
		{"/Users/a", "~/x", "/Users/a/x"},
		{"/Users/a/", "~/x", "/Users/a/x"},
		{"/Users/a//", "~", "/Users/a"},
		{"/Users/a", "~/", "/Users/a/"},
		{"", "~/x", "~/x"},
		{"/Users/a", "~bob/x", "~bob/x"},
		{"/Users/a", "/x/~", "/x/~"},
		{"/Users/a", "x~", "x~"},
		{"/Users/a", "", ""},
		{"/", "~/x", "/x"},
		{"/", "~", "/"},
	};
	for (auto &c : cases)
		CHECK_EQ(expand_home(c.home, c.in), string(c.want));
}

TEST(paths_unquote) {
	struct {
		const char *in, *want;
	} cases[] = {
		{"  'a b'  ", "a b"}, {"\"x\"", "x"},    {"'x\"", "'x\""}, {"'", "'"},       {"\"", "\""},
		{"''", ""},           {"x", "x"},        {"\"'x'\"", "'x'"}, {" \t~/x \n", "~/x"}, {"", ""},
		{"'a'b'", "a'b"},     {"a'", "a'"},
	};
	for (auto &c : cases)
		CHECK_EQ(unquote(c.in), string(c.want));
}

TEST(paths_python_string) {
	struct {
		const char *in, *want;
	} cases[] = {
		{"", "\"\""},
		{"/a/b.wav", "\"/a/b.wav\""},
		{"a\"b", "\"a\\\"b\""},
		{"a\\b", "\"a\\\\b\""},
		{"it's", "\"it's\""},
		{"a\nb\r", "\"a\\nb\\r\""},
		{"\xC3\xA9", "\"\xC3\xA9\""},
	};
	for (auto &c : cases)
		CHECK_EQ(python_string(c.in), string(c.want));
}

TEST(paths_drop_text) {
	CHECK_EQ(drop_text("/Users/a", {"Macintosh HD:/Users/a/x.wav", "~/y z.txt"}),
			 string("\"/Users/a/x.wav\" \"/Users/a/y z.txt\""));
	CHECK_EQ(drop_text("/Users/a", {"/q\"uote"}), string("\"/q\\\"uote\""));
	CHECK_EQ(drop_text("", {"~/x"}), string("\"~/x\""));
	CHECK_EQ(drop_text("/h", {}), string(""));
}
