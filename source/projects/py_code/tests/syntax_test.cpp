// SPDX-License-Identifier: MIT
/* syntax_test.cpp -- headless checks for py_syntax (tree-sitter-python). */

#include "test.h"

#include "py_syntax.h"

using namespace py_code;

/* Role of the byte at `col` on `line`. */
static TokenType role_at(Syntax &s, int line, int col) {
	for (const Token &t : s.tokens(line))
		if (col >= t.col && col < t.col + t.length)
			return t.type;
	return TokenTypeCount;
}

static Syntax &with(Syntax &s, const std::string &text) {
	static int rev = 0;
	s.update(text, ++rev);
	return s;
}

TEST(query_compiles) {
	Syntax s;
	with(s, "x = 1\n");
	/* A broken query leaves every byte Default; `=` must be an operator. */
	CHECK_EQ(role_at(s, 0, 2), Operator);
}

TEST(tokens_cover_line_without_gaps) {
	Syntax s;
	with(s, "def f(a, b=2):\n    return a  # done\n");
	for (int line = 0; line < 2; ++line) {
		int col = 0;
		for (const Token &t : s.tokens(line)) {
			CHECK_EQ(t.col, col);
			col += t.length;
		}
		CHECK_EQ(col, line == 0 ? 14 : 20);
	}
}

TEST(roles) {
	Syntax s;
	with(s, "@dec\n"
			"class Foo(Base):\n"
			"    def m(self, x):\n"
			"        print(len(x), MAX, None, \"a\\n\", 0x1f)  # c\n");
	CHECK_EQ(role_at(s, 0, 0), Decorator);
	CHECK_EQ(role_at(s, 1, 0), Keyword);   /* class */
	CHECK_EQ(role_at(s, 1, 6), Class);     /* Foo */
	CHECK_EQ(role_at(s, 1, 9), Bracket);   /* ( */
	CHECK_EQ(role_at(s, 1, 10), Class);    /* Base */
	CHECK_EQ(role_at(s, 2, 8), Function);  /* m */
	CHECK_EQ(role_at(s, 2, 10), Self);     /* self */
	CHECK_EQ(role_at(s, 3, 8), Builtin);   /* print */
	CHECK_EQ(role_at(s, 3, 14), Builtin);  /* len */
	CHECK_EQ(role_at(s, 3, 22), Number);   /* MAX: constant */
	CHECK_EQ(role_at(s, 3, 27), Builtin);  /* None */
	CHECK_EQ(role_at(s, 3, 33), String);   /* "a */
	CHECK_EQ(role_at(s, 3, 35), Escape);   /* \n */
	CHECK_EQ(role_at(s, 3, 40), Number);   /* 0x1f */
	CHECK_EQ(role_at(s, 3, 47), Comment);
}

TEST(multiline_string_and_fstring) {
	Syntax s;
	with(s, "x = \"\"\"one\n(two\n\"\"\"\ny = f\"{a}\"\n");
	CHECK_EQ(role_at(s, 1, 0), String); /* the ( inside a docstring is text */
	CHECK_EQ(role_at(s, 3, 6), Escape); /* { of the interpolation */
	CHECK_EQ(role_at(s, 3, 7), Default); /* a */
}

TEST(incremental_update) {
	Syntax s;
	s.update("a = 1\n", 1);
	s.update("a = 1\n", 1); /* same revision: no parse */
	CHECK_EQ(s.parses(), 1);
	s.update("a = 12\nb = 'x'\n", 2);
	CHECK_EQ(s.parses(), 2);
	CHECK_EQ(role_at(s, 0, 5), Number);
	CHECK_EQ(role_at(s, 1, 4), String);
	s.update("b = 'x'\n", 3); /* delete the first line */
	CHECK_EQ(role_at(s, 0, 4), String);
	CHECK(!s.has_error());
}

/* The recovered edit must leave the same tree as parsing from scratch. */
TEST(incremental_matches_fresh) {
	const char *steps[] = {
		"",
		"d",
		"def f(x):\n    return x\n",
		"def f(x):\n    s = \"\"\"doc\n    return x\n",   /* opens a string over the rest */
		"def f(x):\n    s = \"\"\"doc\"\"\"\n    return x\n",
		"class A:\n    def f(x):\n        s = \"\"\"doc\"\"\"\n        return x\n",
		"class A:\n    pass\n",
		"x = 1  # \xc3\xa9\ny = '\xc3\xbc'\n",
	};
	Syntax inc;
	int rev = 0;
	for (const char *t : steps) {
		inc.update(t, ++rev);
		Syntax fresh;
		fresh.update(t, 1);
		CHECK_EQ(inc.line_count(), fresh.line_count());
		for (int line = 0; line < fresh.line_count(); ++line) {
			const std::vector<Token> &a = inc.tokens(line), &b = fresh.tokens(line);
			CHECK_EQ(a.size(), b.size());
			for (size_t k = 0; k < a.size() && k < b.size(); ++k) {
				CHECK_EQ(a[k].col, b[k].col);
				CHECK_EQ(a[k].type, b[k].type);
			}
		}
	}
}

TEST(brackets) {
	Syntax s;
	with(s, "f(a[1], {2: '(' })\n");
	BracketMatch m = s.match_bracket(1); /* caret before ( */
	CHECK_EQ(m.first, 1);
	CHECK_EQ(m.second, 17);
	m = s.match_bracket(18); /* caret after ) */
	CHECK_EQ(m.first, 1);
	CHECK_EQ(m.second, 17);
	m = s.match_bracket(3); /* before [ */
	CHECK_EQ(m.first, 3);
	CHECK_EQ(m.second, 5);
	m = s.match_bracket(8); /* before { */
	CHECK_EQ(m.second, 16);
	CHECK(!s.match_bracket(13).valid()); /* the ( inside '(' */
	CHECK(!s.match_bracket(0).valid());
}

TEST(brackets_unbalanced) {
	Syntax s;
	with(s, "f((1)\n");
	CHECK(!s.match_bracket(1).valid());
	BracketMatch m = s.match_bracket(2);
	CHECK_EQ(m.first, 2);
	CHECK_EQ(m.second, 4);
}

static int indent(const std::string &before, const std::string &after = "") {
	Syntax s;
	with(s, before + after);
	return s.indent_at((int)before.size(), 4);
}

TEST(indent_rules) {
	CHECK_EQ(indent("x = 1"), 0);
	CHECK_EQ(indent("def f():"), 4);
	CHECK_EQ(indent("def f():  # c"), 4);
	CHECK_EQ(indent("def f():\n    x = 1"), 4);
	CHECK_EQ(indent("def f():\n    return 1"), 0);
	CHECK_EQ(indent("for a in b:\n    if a:\n        pass"), 4);
	CHECK_EQ(indent("    "), 4);                 /* blank: keep */
	CHECK_EQ(indent("  # note"), 2);             /* comment line: keep */
	CHECK_EQ(indent("foo("), 4);                 /* hanging */
	CHECK_EQ(indent("    foo("), 8);
	CHECK_EQ(indent("foo(a,"), 4);               /* visual: under `a` */
	CHECK_EQ(indent("x = [1,\n     2,"), 5);
	CHECK_EQ(indent("d = {\n    'a': 1,"), 4);
	CHECK_EQ(indent("def f(a,\n      b):"), 4); /* base is the statement's first line */
	CHECK_EQ(indent("x = (1 +\n     2)"), 0);
	CHECK_EQ(indent("s = '''abc", "\n'''"), 0);  /* inside a string: keep */
	CHECK_EQ(indent("    s = '''abc", "\n'''"), 4);
	CHECK_EQ(indent(""), 0);
}

static int reindent(const std::string &before, const std::string &after = "") {
	Syntax s;
	with(s, before + after);
	return s.reindent_at((int)before.size(), 4);
}

TEST(reindent_rules) {
	CHECK_EQ(reindent("if a:\n    x = 1\n    else:"), 0);
	CHECK_EQ(reindent("try:\n    x()\n    except ValueError:"), 0);
	CHECK_EQ(reindent("if a:\n    pass\nelse:"), -1);   /* already right */
	CHECK_EQ(reindent("if a:\n    x = 1\n    y:"), -1); /* not a block keyword */
	CHECK_EQ(reindent("x = [\n    1,\n    ]"), 0);
	CHECK_EQ(reindent("x = [\n    1,\n]"), -1);
	CHECK_EQ(reindent("x = [1, 2]"), -1);
}

static std::string span(const std::string &text, int caret, EvalMode mode, int a = -1, int b = -1) {
	Syntax s;
	with(s, text);
	const EvalSpan e = s.eval_span(caret, a, b, mode);
	return e.has_value() ? text.substr((size_t)e.start, (size_t)(e.end - e.start)) : "<none>";
}

TEST(eval_spans) {
	const std::string doc = "import os\n\n@dec\ndef f():\n    x = 1\n\n    return x\n\n# note\ny = f()\n";
	CHECK_EQ(span(doc, 0, EvalModeBlock), "import os");
	CHECK_EQ(span(doc, doc.find("x = 1"), EvalModeBlock), "@dec\ndef f():\n    x = 1\n\n    return x");
	CHECK_EQ(span(doc, doc.find("\n\n    return") + 1, EvalModeBlock), "@dec\ndef f():\n    x = 1\n\n    return x");
	CHECK_EQ(span(doc, doc.find("x = 1"), EvalModeLine), "    x = 1");
	CHECK_EQ(span(doc, doc.find("# note"), EvalModeBlock), "# note"); /* comment: line fallback */
	CHECK_EQ(span(doc, 10, EvalModeBlock), "<none>");                  /* blank line */
	CHECK_EQ(span(doc, 0, EvalModeLine, 0, 6), "import");
	CHECK_EQ(span(doc, 3, EvalModeDocument, 0, 6), doc);
}

TEST(expressions) {
	Syntax s;
	CHECK(s.is_expression("1 + 1"));
	CHECK(s.is_expression("print('x')"));
	CHECK(s.is_expression("f(a=1)  # kw"));
	CHECK(s.is_expression("a, b"));
	CHECK(!s.is_expression("x = 1"));
	CHECK(!s.is_expression("x += 1"));
	CHECK(!s.is_expression("import os"));
	CHECK(!s.is_expression("def f():\n    pass"));
	CHECK(!s.is_expression("a\nb"));
	CHECK(!s.is_expression("f("));
	CHECK(!s.is_expression(""));
}

TEST(dedent_text) {
	CHECK_EQ(dedent("    x = 1"), "x = 1");
	CHECK_EQ(dedent("    if a:\n        b\n\n    c"), "if a:\n    b\n\nc");
	CHECK_EQ(dedent("x\n  y"), "x\n  y");
	CHECK_EQ(dedent(""), "");
}
