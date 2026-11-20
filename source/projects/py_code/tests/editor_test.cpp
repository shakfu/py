// SPDX-License-Identifier: MIT
/* editor_test.cpp -- pyc::Editor: lines, selection, motion, edits, auto-close, undo. */

#include "test.h"

#include "core/editor.h"

using namespace pyc;
using std::string;

/* `marked` holds the text with `|` at the caret and an optional `^` at the anchor. */
static Editor make(const string &marked) {
	string t;
	int caret = -1, anchor = -1;
	for (char c : marked) {
		if (c == '|')
			caret = (int)t.size();
		else if (c == '^')
			anchor = (int)t.size();
		else
			t += c;
	}
	Editor e(t);
	if (caret < 0)
		caret = (int)t.size();
	e.select(anchor < 0 ? caret : anchor, caret);
	return e;
}

/* The text with `|` at the caret and `^` at the anchor when a selection exists. */
static string show(const Editor &e) {
	string t = e.text();
	if (e.anchor() == e.caret())
		return t.insert((size_t)e.caret(), "|");
	if (e.anchor() > e.caret())
		return t.insert((size_t)e.anchor(), "^").insert((size_t)e.caret(), "|");
	return t.insert((size_t)e.caret(), "|").insert((size_t)e.anchor(), "^");
}

/* Types `keys` (`\b` = backspace) into `marked`. */
static string type(const string &marked, const string &keys, bool autoclose = true) {
	Editor e = make(marked);
	for (char c : keys) {
		if (c == '\b')
			e.backspace(autoclose, false, 4);
		else
			e.type_char((unsigned char)c, autoclose);
	}
	return show(e);
}

/* ---- text and lines ---- */

TEST(editor_lines_empty) {
	Editor e;
	CHECK_EQ(e.length(), 0);
	CHECK_EQ(e.line_count(), 1);
	CHECK_EQ(e.line_start(0), 0);
	CHECK_EQ(e.line_end(0), 0);
	CHECK_EQ(e.line_text(0), string(""));
	CHECK_EQ(e.line_of(0), 0);
	CHECK_EQ(e.cells_before(0), 0);
	CHECK_EQ(e.offset_at_cell(0, 5), 0);
}

TEST(editor_lines) {
	Editor e("a\nbc\n");
	CHECK_EQ(e.line_count(), 3);
	const int starts[] = {0, 2, 5}, ends[] = {1, 4, 5};
	for (int l = 0; l < 3; ++l) {
		CHECK_EQ(e.line_start(l), starts[l]);
		CHECK_EQ(e.line_end(l), ends[l]);
	}
	CHECK_EQ(e.line_text(1), string("bc"));
	CHECK_EQ(e.line_text(2), string(""));
	const int line_of[] = {0, 0, 1, 1, 1, 2};
	for (int off = 0; off <= 5; ++off)
		CHECK_EQ(e.line_of(off), line_of[off]);
	CHECK_EQ(e.line_start(-1), 0); /* clamped */
	CHECK_EQ(e.line_start(9), 5);
}

TEST(editor_cells_utf8) {
	/* a(1) e-acute(2) euro(3) emoji(4) b(1) */
	Editor e("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80" "b\nx");
	const int offs[] = {0, 1, 3, 6, 10, 11};
	for (int cell = 0; cell < 6; ++cell) {
		CHECK_EQ(e.offset_at_cell(0, cell), offs[cell]);
		CHECK_EQ(e.cells_before(offs[cell]), cell);
	}
	CHECK_EQ(e.offset_at_cell(0, 99), 11); /* clamped to the line */
	CHECK_EQ(e.offset_at_cell(1, 99), 13);
	CHECK_EQ(e.offset_at_cell(0, -2), 0);
	CHECK_EQ(e.cells_before(2), 1); /* inside e-acute: snapped back */
	CHECK_EQ(e.cells_before(8), 3); /* inside the emoji */
	CHECK_EQ(e.cells_before(13), 1);
}

TEST(editor_invalid_utf8) {
	/* stray continuation, 0xFF, truncated 3-byte lead */
	Editor e("a\x80\xFF\xE2\x82x");
	CHECK_EQ(e.cells_before(e.length()), 6);
	int steps = 0;
	e.set_caret(0);
	while (e.caret() < e.length() && steps < 100) {
		e.move(Move::Right);
		++steps;
	}
	CHECK_EQ(steps, 6);
	while (e.caret() > 0 && steps < 100) {
		e.move(Move::Left);
		++steps;
	}
	CHECK_EQ(steps, 12);
	CHECK_EQ(utf8_encode(0x41), string("A"));
	CHECK_EQ(utf8_encode(0xE9), string("\xC3\xA9"));
	CHECK_EQ(utf8_encode(0x20AC), string("\xE2\x82\xAC"));
	CHECK_EQ(utf8_encode(0x1F600), string("\xF0\x9F\x98\x80"));
	CHECK_EQ(utf8_encode(0xD800), string(""));
	CHECK_EQ(utf8_encode(0x110000), string(""));
}

TEST(editor_revision) {
	Editor e("ab");
	const int r0 = e.revision();
	e.set_caret(1);
	e.select_all();
	e.move(Move::DocEnd);
	CHECK_EQ(e.revision(), r0); /* selection never bumps it */
	e.insert("c");
	CHECK_EQ(e.revision(), r0 + 1);
	CHECK(!e.insert("")); /* no change, no bump */
	CHECK_EQ(e.revision(), r0 + 1);
	e.undo();
	CHECK_EQ(e.revision(), r0 + 2);
	e.redo();
	CHECK_EQ(e.revision(), r0 + 3);
	e.reset("x");
	CHECK_EQ(e.revision(), r0 + 4);
}

/* ---- selection ---- */

TEST(editor_set_caret_clamps) {
	Editor e("a\xC3\xA9");
	e.set_caret(100);
	CHECK_EQ(e.caret(), 3);
	e.set_caret(-5);
	CHECK_EQ(e.caret(), 0);
	e.set_caret(2); /* inside e-acute */
	CHECK_EQ(e.caret(), 1);
	e.set_caret(3, true);
	CHECK_EQ(e.anchor(), 1);
	CHECK_EQ(e.selected_text(), string("\xC3\xA9"));
	e.select(9, -9);
	CHECK_EQ(e.anchor(), 3);
	CHECK_EQ(e.caret(), 0);
	CHECK_EQ(e.sel_start(), 0);
	CHECK_EQ(e.sel_end(), 3);
	e.select_all();
	CHECK_EQ(show(e), string("^a\xC3\xA9|"));
}

TEST(editor_select_word_at) {
	struct {
		const char *text;
		int off;
		const char *want;
	} cases[] = {
		{"foo_1 += bar\n  x", 1, "^foo_1| += bar\n  x"},
		{"foo_1 += bar\n  x", 0, "^foo_1| += bar\n  x"},
		{"foo_1 += bar\n  x", 6, "foo_1 ^+=| bar\n  x"},     /* punctuation run */
		{"foo_1 += bar\n  x", 5, "foo_1^ |+= bar\n  x"},     /* whitespace run */
		{"foo_1 += bar\n  x", 12, "foo_1 += bar|\n  x"},    /* newline: nothing */
		{"foo_1 += bar\n  x", 13, "foo_1 += bar\n^  |x"},
		{"foo_1 += bar\n  x", 16, "foo_1 += bar\n  x|"},    /* end of text: nothing */
		{"a\xC3\xA9+b", 1, "a^\xC3\xA9+|b"},                /* non-ASCII is not a word char */
		{"a\t \tb", 2, "a^\t \t|b"},
		{"", 0, "|"},
	};
	for (auto &c : cases) {
		Editor e(c.text);
		e.select_word_at(c.off);
		CHECK_EQ(show(e), string(c.want));
	}
}

TEST(editor_select_line_at) {
	Editor e("ab\ncd\n");
	e.select_line_at(4);
	CHECK_EQ(show(e), string("ab\n^cd|\n"));
	e.select_line_at(6);
	CHECK_EQ(show(e), string("ab\ncd\n|"));
	e.select_line_at(0);
	CHECK_EQ(show(e), string("^ab|\ncd\n"));
}

/* ---- motion ---- */

static string moved(const string &marked, Move m, bool extend = false, int times = 1) {
	Editor e = make(marked);
	for (int i = 0; i < times; ++i)
		e.move(m, extend);
	return show(e);
}

TEST(editor_move_left_right) {
	CHECK_EQ(moved("a\xC3\xA9|b", Move::Left), string("a|\xC3\xA9" "b"));
	CHECK_EQ(moved("a|\xC3\xA9" "b", Move::Right), string("a\xC3\xA9|b"));
	CHECK_EQ(moved("|ab", Move::Left), string("|ab"));
	CHECK_EQ(moved("ab|", Move::Right), string("ab|"));
	CHECK_EQ(moved("a\n|b", Move::Left), string("a|\nb"));
	CHECK_EQ(moved("a^bc|d", Move::Left), string("a|bcd"));  /* collapse to the left side */
	CHECK_EQ(moved("a|bc^d", Move::Right), string("abc|d")); /* collapse to the right side */
	CHECK_EQ(moved("a^bc|d", Move::Right), string("abc|d"));
	CHECK_EQ(moved("a^bc|d", Move::Left, true), string("a^b|cd")); /* extend */
	CHECK_EQ(moved("a|bcd", Move::Right, true, 2), string("a^bc|d"));
}

TEST(editor_move_words) {
	const char *t = "foo.bar  baz";
	const int right[] = {3, 4, 7, 12, 12};
	Editor e(t);
	for (int want : right) {
		e.move(Move::WordRight);
		CHECK_EQ(e.caret(), want);
	}
	const int left[] = {9, 4, 3, 0, 0};
	for (int want : left) {
		e.move(Move::WordLeft);
		CHECK_EQ(e.caret(), want);
	}
	CHECK_EQ(moved("a|\n  b", Move::WordRight), string("a\n  b|"));
	CHECK_EQ(moved("a\n  |b", Move::WordLeft), string("|a\n  b"));
	CHECK_EQ(moved("x = (|a)", Move::WordRight), string("x = (a|)"));
	CHECK_EQ(moved("x += |", Move::WordLeft), string("x |+= "));
	CHECK_EQ(moved("|foo bar", Move::WordRight, true), string("^foo| bar"));
}

TEST(editor_move_vertical_goal) {
	Editor e("abcdef\nab\nabcdef");
	e.set_caret(5);
	e.move(Move::Down);
	CHECK_EQ(e.caret(), 9); /* clamped to "ab" */
	e.move(Move::Down);
	CHECK_EQ(e.caret(), 15); /* goal cell 5 kept */
	e.move(Move::Up);
	CHECK_EQ(e.caret(), 9);
	e.move(Move::Up);
	CHECK_EQ(e.caret(), 5);
	e.set_caret(15);
	e.move(Move::Left); /* new goal 4 */
	e.move(Move::Up);
	e.move(Move::Up);
	CHECK_EQ(e.caret(), 4);
	/* past the first/last line */
	CHECK_EQ(moved("ab|c\nd", Move::Up), string("|abc\nd"));
	CHECK_EQ(moved("abc\n|d", Move::Down), string("abc\nd|"));
	CHECK_EQ(moved("", Move::Down), string("|"));
	/* cells, not bytes */
	CHECK_EQ(moved("\xC3\xA9\xC3\xA9|\nabc", Move::Down), string("\xC3\xA9\xC3\xA9\nab|c"));
	CHECK_EQ(moved("ab^c\nd|ef", Move::Up, true), string("a|b^c\ndef"));
}

TEST(editor_move_goal_after_doc_start) {
	Editor e("abc\nabcdef");
	e.set_caret(9);
	e.move(Move::Up);
	CHECK_EQ(e.caret(), 3);
	e.move(Move::Up);
	CHECK_EQ(e.caret(), 0);
	e.move(Move::Down);
	CHECK_EQ(e.caret(), 4 + 5); /* goal survives the doc start */
}

TEST(editor_move_line_doc) {
	CHECK_EQ(moved("    x = 1|", Move::LineStart), string("    |x = 1"));
	CHECK_EQ(moved("    |x = 1", Move::LineStart), string("|    x = 1"));
	CHECK_EQ(moved("|    x = 1", Move::LineStart), string("    |x = 1"));
	CHECK_EQ(moved("  |  x", Move::LineStart), string("    |x"));
	CHECK_EQ(moved(" |  ", Move::LineStart), string("   |")); /* blank line: end, then 0 */
	CHECK_EQ(moved("   |", Move::LineStart), string("|   "));
	CHECK_EQ(moved("a\n\tb|", Move::LineStart), string("a\n\t|b"));
	CHECK_EQ(moved("a|b\ncd", Move::LineEnd), string("ab|\ncd"));
	CHECK_EQ(moved("ab\nc|d", Move::DocStart), string("|ab\ncd"));
	CHECK_EQ(moved("a|b\ncd", Move::DocEnd), string("ab\ncd|"));
	CHECK_EQ(moved("a|b\ncd", Move::DocEnd, true), string("a^b\ncd|"));
	CHECK_EQ(moved("ab\n  c|d", Move::LineStart, true), string("ab\n  |c^d"));
}

/* ---- edits ---- */

TEST(editor_insert) {
	Editor e = make("a^bc|d");
	CHECK(e.insert("XY"));
	CHECK_EQ(show(e), string("aXY|d"));
	CHECK(!e.insert(""));
	e.select(0, 1);
	CHECK(e.insert("")); /* deletes the selection */
	CHECK_EQ(show(e), string("|XYd"));
	CHECK(e.insert("1\n2"));
	CHECK_EQ(show(e), string("1\n2|XYd"));
	CHECK_EQ(e.line_count(), 2);
}

TEST(editor_type_char_plain) {
	CHECK_EQ(type("ab|", "c", false), string("abc|"));
	CHECK_EQ(type("a^b|c", "x", false), string("ax|c"));
	CHECK_EQ(type("a^b|c", "(", false), string("a(|c"));
	Editor e;
	CHECK(e.type_char(0xE9, false));
	CHECK(e.type_char(0x1F600, true));
	CHECK_EQ(e.text(), string("\xC3\xA9\xF0\x9F\x98\x80"));
	CHECK_EQ(e.caret(), 6);
	CHECK(!e.type_char(0, true));
	CHECK(!e.type_char(0xDC00, true));
}

TEST(editor_backspace) {
	struct {
		const char *in;
		bool word;
		const char *want;
	} cases[] = {
		{"ab|", false, "a|"},
		{"a\xC3\xA9|", false, "a|"},
		{"|ab", false, "|ab"},
		{"a^bc|d", false, "a|d"},
		{"a\n|b", false, "a|b"},
		{"foo bar|", true, "foo |"},
		{"foo bar  |", true, "foo |"},
		{"x = (a|", true, "x = (|"},
		{"a\n|", true, "|"},
		{"      |", false, "    |"},  /* leading spaces: back to the tab stop */
		{"    |", false, "|"},
		{"     |", false, "    |"},
		{"  |x", false, "|x"},
		{"a\n  |  b", false, "a\n|  b"},
		{"a   |", false, "a  |"},     /* not leading */
		{"\t  |", false, "\t |"},     /* tab: not only spaces */
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		const bool changed = e.backspace(true, c.word, 4);
		CHECK_EQ(show(e), string(c.want));
		CHECK_EQ(changed, string(c.in) != string(c.want));
	}
	Editor e = make("   |");
	e.backspace(false, false, 2); /* tabwidth is passed in */
	CHECK_EQ(show(e), string("  |"));
	Editor z = make("  |");
	z.backspace(false, false, 0); /* tabwidth < 1 acts as 1 */
	CHECK_EQ(show(z), string(" |"));
}

TEST(editor_delete_forward) {
	struct {
		const char *in;
		bool word;
		const char *want;
	} cases[] = {
		{"|ab", false, "|b"},
		{"a|\xC3\xA9" "b", false, "a|b"},
		{"ab|", false, "ab|"},
		{"a|\nb", false, "a|b"},
		{"a^bc|d", false, "a|d"},
		{"|foo bar", true, "| bar"},
		{"foo| bar", true, "foo|"},
		{"|(x)", true, "|x)"},
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		const bool changed = e.delete_forward(c.word);
		CHECK_EQ(show(e), string(c.want));
		CHECK_EQ(changed, string(c.in) != string(c.want));
	}
}

TEST(editor_newline) {
	struct {
		const char *in;
		int indent;
		const char *want;
	} cases[] = {
		{"ab|cd", 4, "ab\n    |cd"},
		{"ab|", 0, "ab\n|"},
		{"a^bc|d", 2, "a\n  |d"},
		{"f(|)", 4, "f(\n    |\n)"},
		{"f[|]", 4, "f[\n    |\n]"},
		{"f{|}", 4, "f{\n    |\n}"},
		{"f( | )", 4, "f(\n    |\n)"},          /* spaces between are dropped */
		{"    g([|])", 8, "    g([\n        |\n    ])"}, /* closer indented like the opener's line */
		{"f(|]", 4, "f(\n    |]"},
		{"'|'", 0, "'\n|'"},
		{"f(x|)", 4, "f(x\n    |)"},
		{"f(^abc|)", 4, "f(\n    |\n)"},
		{"", -3, "\n|"},
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		CHECK(e.newline(c.indent));
		CHECK_EQ(show(e), string(c.want));
	}
}

TEST(editor_indent) {
	struct {
		const char *in;
		int tw;
		const char *want;
	} cases[] = {
		{"ab|", 4, "ab  |"},
		{"|x", 4, "    |x"},
		{"abcd|", 4, "abcd    |"},
		{"\xC3\xA9|", 4, "\xC3\xA9   |"},              /* cells, not bytes */
		{"a^b|c", 4, "a   |c"},                       /* selection inside a line: replaced */
		{"ab|", 3, "ab |"},
		{"^ab\ncd\nef|", 4, "^    ab\n    cd\n    ef|"},
		{"^ab\ncd\n|ef", 4, "^    ab\n    cd\n|ef"},  /* last line at column 0 excluded */
		{"a^b\ncd|", 2, "  a^b\n  cd|"},
		{"|ab\ncd^", 4, "|    ab\n    cd^"},          /* direction kept */
		{"^a\n\nb|", 2, "^  a\n  \n  b|"},
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		CHECK(e.indent(c.tw));
		CHECK_EQ(show(e), string(c.want));
	}
}

TEST(editor_outdent) {
	struct {
		const char *in;
		const char *want;
	} cases[] = {
		{"    |x", "|x"},
		{"      x|", "  x|"},
		{"  |x", "|x"},
		{" | x", "|x"},  /* caret inside the indent */
		{"|x", "|x"},
		{"\tx|", "\tx|"},  /* only spaces */
		{"a\n    b|\nc", "a\nb|\nc"},
		{"^    a\n  b\nc|", "^a\nb\nc|"},
		{"^    a\n    b\n|c", "^a\nb\n|c"},
		{"  ^    a\n    b|", "^  a\nb|"},
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		const bool changed = e.outdent(4);
		CHECK_EQ(show(e), string(c.want));
		CHECK_EQ(changed, string(c.in) != string(c.want));
	}
}

TEST(editor_set_line_indent) {
	struct {
		const char *in;
		int line, spaces;
		bool changed;
		const char *want;
	} cases[] = {
		{"  x|", 0, 4, true, "    x|"},
		{"    else:|", 0, 0, true, "else:|"},
		{"a\n  b|", 1, 0, true, "a\nb|"},
		{"\t x|", 0, 2, true, "  x|"},
		{"  x|", 0, 2, false, "  x|"},
		{"    |x", 0, 2, true, "  |x"},
		{"   | x", 0, 1, true, " |x"},       /* inside the indent: clamped */
		{"|  x\ny", 0, 0, true, "|x\ny"},
		{"  x\n|y", 0, 0, true, "x\n|y"},
		{"x|", 3, 4, false, "x|"},
	};
	for (auto &c : cases) {
		Editor e = make(c.in);
		CHECK_EQ(e.set_line_indent(c.line, c.spaces), c.changed);
		CHECK_EQ(show(e), string(c.want));
	}
}

TEST(editor_reset) {
	Editor e = make("abc|");
	e.insert("d");
	e.reset("x\ny");
	CHECK_EQ(show(e), string("|x\ny"));
	CHECK_EQ(e.line_count(), 2);
	CHECK(!e.can_undo());
	CHECK(!e.can_redo());
	CHECK(!e.undo());
}

/* ---- auto-close (ported from buffer_test.cpp) ---- */

TEST(pairs) {
	CHECK_EQ(type("", "("), string("(|)"));
	CHECK_EQ(type("", "(x)"), string("(x)|")); /* type-over */
	CHECK_EQ(type("", "["), string("[|]"));
	CHECK_EQ(type("", "{"), string("{|}"));
	CHECK_EQ(type("", "[x]"), string("[x]|"));
	CHECK_EQ(type("", "{x}"), string("{x}|"));
	CHECK_EQ(type("", "(", false), string("(|"));
	CHECK_EQ(type("", ")"), string(")|")); /* lone closer */
	CHECK_EQ(type("(|]", ")"), string("()|]"));
	CHECK_EQ(type("(|)", ")", false), string("()|)"));
	CHECK_EQ(type("|  ", "("), string("(|)  "));
	CHECK_EQ(type("|)", "("), string("(|))"));
}

TEST(no_pair_before_word) {
	CHECK_EQ(type("|foo", "("), string("(|foo"));
	CHECK_EQ(type("|_x", "["), string("[|_x"));
	CHECK_EQ(type("|1", "{"), string("{|1"));
	CHECK_EQ(type("|.x", "("), string("(|).x"));
}

TEST(quotes) {
	CHECK_EQ(type("", "'"), string("'|'"));
	CHECK_EQ(type("", "'a'"), string("'a'|"));
	CHECK_EQ(type("don", "'"), string("don'|")); /* apostrophe after a word */
	CHECK_EQ(type("x = f", "\""), string("x = f\"|\"")); /* string prefix */
	CHECK_EQ(type("x = rb", "'"), string("x = rb'|'"));
	CHECK_EQ(type("x = Rb", "'"), string("x = Rb'|'"));
	CHECK_EQ(type("x = fr", "'"), string("x = fr'|'"));
	CHECK_EQ(type("x = u", "'"), string("x = u'|'"));
	CHECK_EQ(type("x = t", "'"), string("x = t'|'"));
	CHECK_EQ(type("x = abc", "'"), string("x = abc'|"));
	CHECK_EQ(type("x = rbf", "'"), string("x = rbf'|")); /* three chars */
	CHECK_EQ(type("x = x", "'"), string("x = x'|"));
	CHECK_EQ(type("x = _", "'"), string("x = _'|"));
	CHECK_EQ(type("x = 1", "'"), string("x = 1'|"));
	CHECK_EQ(type("x = (", "'"), string("x = ('|'"));
	CHECK_EQ(type("", "\"\"\""), string("\"\"\"|\"\"\"")); /* triple */
	CHECK_EQ(type("", "'''"), string("'''|'''"));
	CHECK_EQ(type("'abc|'", "'"), string("'abc'|"));
	CHECK_EQ(type("|x", "'"), string("'|x")); /* before a word: quote alone */
	CHECK_EQ(type("", "'", false), string("'|"));
}

TEST(backspace_pair) {
	CHECK_EQ(type("", "(\b"), string("|"));
	CHECK_EQ(type("", "'\b"), string("|"));
	CHECK_EQ(type("", "(x\b\b"), string("|"));
	CHECK_EQ(type("ab", "\b"), string("a|"));
	CHECK_EQ(type("[|]", "\b"), string("|"));
	CHECK_EQ(type("{|}", "\b"), string("|"));
	CHECK_EQ(type("\"|\"", "\b"), string("|"));
	CHECK_EQ(type("(|]", "\b"), string("|]"));
	CHECK_EQ(type("(|)", "\b", false), string("|)"));
}

TEST(wrap_selection) {
	Editor b("x = a + b");
	b.select(4, 9);
	b.type_char('(', true);
	CHECK_EQ(b.text(), string("x = (a + b)"));
	CHECK_EQ(b.selected_text(), string("a + b"));
	CHECK_EQ(type("^ab|", "["), string("[^ab|]"));
	CHECK_EQ(type("^ab|", "{"), string("{^ab|}"));
	CHECK_EQ(type("^ab|", "'"), string("'^ab|'"));
	CHECK_EQ(type("^ab|", "\""), string("\"^ab|\""));
	CHECK_EQ(type("|ab^", "("), string("(|ab^)")); /* direction kept */
	CHECK_EQ(type("^ab|", ")"), string(")|"));     /* closer replaces */
	CHECK_EQ(type("^ab|", "(", false), string("(|"));
	CHECK_EQ(type("^ab|", "(("), string("((^ab|))"));
}

TEST(undo_pair_is_one_step) {
	Editor b("");
	b.type_char('(', true);
	CHECK(b.undo());
	CHECK_EQ(b.text(), string(""));
	CHECK(!b.can_undo());
}

TEST(newline_moves_closer) {
	Editor b("f()");
	b.set_caret(2);
	b.newline(4);
	CHECK_EQ(b.text(), string("f(\n    \n)"));
	CHECK_EQ(b.caret(), 7);
}

/* ---- undo ---- */

/* Undoes once per entry and checks the text after each step. */
static void undo_steps(Editor &e, std::vector<string> want) {
	for (const string &w : want) {
		CHECK(e.undo());
		CHECK_EQ(show(e), w);
	}
	CHECK(!e.undo());
}

TEST(undo_typing_word_by_word) {
	Editor e;
	for (char c : string("foo bar"))
		e.type_char((unsigned char)c, true);
	undo_steps(e, {"foo |", "|"});
}

TEST(undo_typing_runs_of_spaces) {
	Editor e;
	for (char c : string("foo  bar baz"))
		e.type_char((unsigned char)c, true);
	undo_steps(e, {"foo  bar |", "foo |", "|"});
}

TEST(undo_typing_leading_space) {
	Editor e;
	for (char c : string("  ab"))
		e.type_char((unsigned char)c, true);
	undo_steps(e, {"|"});
}

TEST(undo_typing_broken_by_motion) {
	Editor e;
	e.type_char('a', true);
	e.type_char('b', true);
	e.move(Move::Left);
	e.move(Move::Right);
	e.type_char('c', true);
	e.set_caret(3);
	e.type_char('d', true);
	e.select(4, 4);
	e.type_char('e', true);
	undo_steps(e, {"abcd|", "abc|", "ab|", "|"});
}

TEST(undo_typing_over_selection_new_group) {
	Editor e = make("ab|");
	e.type_char('c', true);
	e.select(0, 1);
	e.type_char('x', true);
	e.type_char('y', true);
	undo_steps(e, {"^a|bc", "ab|"});
}

TEST(undo_type_over_keeps_group) {
	Editor e;
	for (char c : string("(x)y"))
		e.type_char((unsigned char)c, true);
	CHECK_EQ(e.text(), string("(x)y"));
	undo_steps(e, {"(|)", "|"});
}

TEST(undo_backspace_and_delete_groups) {
	Editor e = make("abc|def");
	e.backspace(true, false, 4);
	e.backspace(true, false, 4);
	e.delete_forward(false);
	e.delete_forward(false);
	e.backspace(true, false, 4);
	CHECK_EQ(show(e), string("|f"));
	undo_steps(e, {"a|f", "a|def", "abc|def"});
}

TEST(undo_typing_then_backspace) {
	Editor e;
	e.type_char('a', true);
	e.type_char('b', true);
	e.backspace(true, false, 4);
	e.type_char('c', true);
	undo_steps(e, {"a|", "ab|", "|"});
}

TEST(undo_own_groups) {
	Editor e;
	e.insert("x");
	e.insert("y");
	e.newline(0);
	e.newline(0);
	e.indent(4);
	e.indent(4);
	e.type_char('(', true);
	e.type_char('[', true);
	CHECK_EQ(show(e), string("xy\n\n        ([|])"));
	undo_steps(e, {"xy\n\n        (|)", "xy\n\n        |", "xy\n\n    |", "xy\n\n|", "xy\n|", "xy|", "x|", "|"});
}

TEST(undo_wrap_is_one_group) {
	Editor e = make("^ab|");
	e.type_char('(', true);
	e.type_char('"', true);
	undo_steps(e, {"(^ab|)", "^ab|"});
}

TEST(undo_set_line_indent_joins) {
	Editor e = make("    |");
	for (char c : string("else:"))
		e.type_char((unsigned char)c, true);
	CHECK(e.set_line_indent(0, 0));
	CHECK_EQ(show(e), string("else:|"));
	undo_steps(e, {"    |"});
	CHECK(e.redo());
	CHECK_EQ(show(e), string("else:|"));

	/* joins a pair group as well */
	Editor p = make("    x = f|");
	p.type_char('(', true);
	p.set_line_indent(0, 0);
	undo_steps(p, {"    x = f|"});

	/* after a move: its own group */
	Editor m = make("  x|");
	m.type_char('y', true);
	m.move(Move::Left);
	m.set_line_indent(0, 0);
	undo_steps(m, {"  x|y", "  x|"});

	/* after an undo: its own group */
	Editor u = make("a\n  b|");
	u.type_char('c', true);
	u.insert("d");
	u.undo();
	u.set_line_indent(1, 0);
	undo_steps(u, {"a\n  bc|", "a\n  b|"});

	/* on a fresh document */
	Editor f = make("  x|");
	f.set_line_indent(0, 0);
	undo_steps(f, {"  x|"});
}

TEST(undo_set_line_indent_then_typing) {
	Editor e = make("    |");
	e.type_char(')', true);
	e.set_line_indent(0, 0);
	e.type_char('x', true);
	undo_steps(e, {"    |"});
}

TEST(undo_restores_selection) {
	Editor e = make("^abc|");
	e.insert("x");
	CHECK(e.undo());
	CHECK_EQ(show(e), string("^abc|"));
	CHECK(e.can_redo());
	CHECK(e.redo());
	CHECK_EQ(show(e), string("x|"));
	CHECK(!e.redo());

	Editor r = make("|cba^");
	r.backspace(true, false, 4);
	r.undo();
	CHECK_EQ(show(r), string("|cba^"));

	Editor w = make("x ^ab| y");
	w.type_char('[', true);
	w.undo();
	w.redo();
	CHECK_EQ(show(w), string("x [^ab|] y"));
}

TEST(undo_multi_record_group_redo) {
	Editor e = make("a|");
	for (char c : string("bcd"))
		e.type_char((unsigned char)c, true);
	e.undo();
	CHECK_EQ(show(e), string("a|"));
	e.redo();
	CHECK_EQ(show(e), string("abcd|"));
	e.undo();
	e.undo();
	CHECK_EQ(show(e), string("a|"));
}

TEST(undo_new_edit_clears_redo) {
	Editor e;
	e.insert("a");
	e.insert("b");
	e.undo();
	CHECK(e.can_redo());
	e.insert("c");
	CHECK(!e.can_redo());
	CHECK_EQ(e.text(), string("ac"));
	CHECK(!e.redo());
	CHECK(!Editor().can_undo());
	CHECK(!Editor().undo());
	CHECK(!Editor().redo());
}

TEST(undo_redo_indent_block) {
	Editor e = make("^ab\ncd\nef|");
	e.indent(4);
	e.outdent(2);
	CHECK_EQ(show(e), string("^  ab\n  cd\n  ef|"));
	undo_steps(e, {"^    ab\n    cd\n    ef|", "^ab\ncd\nef|"});
	e.redo();
	e.redo();
	CHECK_EQ(show(e), string("^  ab\n  cd\n  ef|"));
}

TEST(undo_history_limit) {
	Editor e;
	for (int i = 0; i < Editor::kHistoryLimit + 1; ++i)
		e.insert("x");
	int n = 0;
	while (e.undo())
		++n;
	CHECK_EQ(n, Editor::kHistoryLimit);
	CHECK_EQ(e.text(), string("x")); /* the oldest group was dropped */
}

TEST(undo_history_drops_whole_groups) {
	Editor e;
	for (int i = 0; i < 600; ++i)
		e.type_char('a', true);
	e.move(Move::Left);
	for (int i = 0; i < 600; ++i)
		e.type_char('b', true);
	CHECK(e.undo());
	CHECK_EQ(e.text(), string(600, 'a'));
	CHECK(!e.undo()); /* the 600-record 'a' group went whole */

	Editor big; /* a single group larger than the limit is kept */
	for (int i = 0; i < 1500; ++i)
		big.type_char('a', true);
	CHECK(big.undo());
	CHECK_EQ(big.text(), string(""));
}
