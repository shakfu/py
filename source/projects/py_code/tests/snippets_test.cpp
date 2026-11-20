// SPDX-License-Identifier: MIT
/* snippets_test.cpp -- snippet file parsing and `snip.<key>` matching. */

#include "test.h"

#include "core/snippets.h"

using namespace pyc;
using std::string;

static string body(const Snippets &s, const string &key) {
	const string *b = s.find(key);
	return b ? *b : string("<none>");
}

static string joined(const std::vector<string> &v) {
	string o;
	for (const string &s : v)
		o += (o.empty() ? "" : ",") + s;
	return o;
}

TEST(snippets_basic) {
	const Snippets s = snippets::parse("preamble ignored\n"
									   "### sine\n"
									   "import math\n"
									   "math.sin(0)\n"
									   "### b_2\n"
									   "x = 1\n");
	CHECK_EQ(joined(s.keys()), string("sine,b_2"));
	CHECK_EQ(s.size(), (size_t)2);
	CHECK_EQ(body(s, "sine"), string("import math\nmath.sin(0)"));
	CHECK_EQ(body(s, "b_2"), string("x = 1"));
	CHECK_EQ(body(s, "preamble"), string("<none>"));
	CHECK(s.find("missing") == nullptr);
	CHECK(s.duplicates.empty());
	CHECK(s.empty.empty());
}

TEST(snippets_headers) {
	struct {
		const char *text;
		const char *keys;
		const char *first_body;
	} cases[] = {
		{"### a  \t\nx", "a", "x"},              /* rest of the line trimmed */
		{"###  a\nx", "a", "x"},
		{"### a\n###b\nx", "a", "###b\nx"},      /* no space: body line */
		{"### a\n### b c\nx", "a", "### b c\nx"}, /* not a valid key: body line */
		{"### a\n### b-c\nx", "a", "### b-c\nx"},
		{"### a\n#### b\nx", "a", "#### b\nx"},
		{"### a\n### \nx", "a", "### \nx"},
		{"### a\n ### b\nx", "a", " ### b\nx"},  /* must start the line */
		{"## a\nx", "", ""},
		{"", "", ""},
		{"no headers at all\n", "", ""},
	};
	for (auto &c : cases) {
		const Snippets s = snippets::parse(c.text);
		CHECK_EQ(joined(s.keys()), string(c.keys));
		if (!s.entries.empty())
			CHECK_EQ(s.entries[0].second, string(c.first_body));
	}
}

TEST(snippets_line_endings_bom) {
	const Snippets crlf = snippets::parse("### a\r\nx\r\ny\r\n### b\rz\r");
	CHECK_EQ(body(crlf, "a"), string("x\ny"));
	CHECK_EQ(body(crlf, "b"), string("z"));
	const Snippets bom = snippets::parse("\xEF\xBB\xBF### a\nx\n");
	CHECK_EQ(joined(bom.keys()), string("a"));
	const Snippets utf8 = snippets::parse("### e\nprint('\xC3\xA9')\n");
	CHECK_EQ(body(utf8, "e"), string("print('\xC3\xA9')"));
}

TEST(snippets_body_trim) {
	const Snippets s = snippets::parse("### a\n"
									   "\n"
									   "   \n"
									   "def f():\n"
									   "\n"
									   "    return 1\n"
									   "\t\n"
									   "\n"
									   "### b\n"
									   "    indented\n");
	CHECK_EQ(body(s, "a"), string("def f():\n\n    return 1"));
	CHECK_EQ(body(s, "b"), string("    indented"));
}

TEST(snippets_duplicates_and_empty) {
	const Snippets s = snippets::parse("### a\nfirst\n"
									   "### e\n\n  \n"
									   "### a\nsecond\n"
									   "### f\n"
									   "### a\nthird\n"
									   "### e\nlate body\n");
	CHECK_EQ(body(s, "a"), string("first"));
	CHECK_EQ(joined(s.keys()), string("a"));
	CHECK_EQ(joined(s.duplicates), string("a,e")); /* each once */
	CHECK_EQ(joined(s.empty), string("e,f"));
	CHECK(s.find("e") == nullptr);                 /* first occurrence wins, even empty */
	const Snippets tail = snippets::parse("### z");
	CHECK_EQ(joined(tail.empty), string("z"));
}

TEST(snippets_match) {
	struct {
		const char *code;
		bool ok;
		const char *key;
	} cases[] = {
		{"snip.foo", true, "foo"},     {"  snip.foo \n", true, "foo"}, {"\tsnip.A_1\n\n", true, "A_1"},
		{"snip.", false, ""},          {"snip.foo bar", false, ""},    {"snip.foo()", false, ""},
		{"x.snip.foo", false, ""},     {"snip.a-b", false, ""},        {"snip foo", false, ""},
		{"Snip.foo", false, ""},       {"", false, ""},                {"snip.foo.bar", false, ""},
	};
	for (auto &c : cases) {
		string key = "unset";
		CHECK_EQ(snippets::match(c.code, &key), c.ok);
		CHECK_EQ(key, string(c.ok ? c.key : "unset"));
	}
	CHECK(snippets::match("snip.x", nullptr));
	CHECK(snippets::valid_key("a_Z9"));
	CHECK(!snippets::valid_key(""));
	CHECK(!snippets::valid_key("a.b"));
}
