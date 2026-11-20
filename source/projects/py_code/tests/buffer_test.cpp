// SPDX-License-Identifier: MIT
/* buffer_test.cpp -- py_buffer: auto-close pairs and basic editing. */

#include "test.h"

#include "py_buffer.h"

using namespace py_code;

/* Types `keys` into a buffer holding `text` with the caret at the end. Returns the
 * text with `|` at the caret. */
static std::string type(const std::string &text, const std::string &keys, bool autoclose = true) {
	Buffer b(text);
	b.set_auto_close(autoclose);
	b.move_document_end();
	for (char c : keys) {
		if (c == '\b')
			b.backspace();
		else
			b.insert_char((unsigned char)c);
	}
	std::string out = b.text();
	out.insert((size_t)b.caret(), "|");
	return out;
}

TEST(pairs) {
	CHECK_EQ(type("", "("), std::string("(|)"));
	CHECK_EQ(type("", "(x)"), std::string("(x)|"));    /* type-over */
	CHECK_EQ(type("", "["), std::string("[|]"));
	CHECK_EQ(type("", "{"), std::string("{|}"));
	CHECK_EQ(type("", "("), std::string("(|)"));
	CHECK_EQ(type("", "(", false), std::string("(|"));
	CHECK_EQ(type("", ")"), std::string(")|"));         /* lone closer */
}

TEST(no_pair_before_word) {
	Buffer b("foo");
	b.set_auto_close(true);
	b.set_caret(0);
	b.insert_char('(');
	CHECK_EQ(b.text(), std::string("(foo"));
}

TEST(quotes) {
	CHECK_EQ(type("", "'"), std::string("'|'"));
	CHECK_EQ(type("", "'a'"), std::string("'a'|"));
	CHECK_EQ(type("don", "'"), std::string("don'|"));  /* apostrophe after a word */
	CHECK_EQ(type("x = f", "\""), std::string("x = f\"|\""));   /* string prefix */
	CHECK_EQ(type("x = rb", "'"), std::string("x = rb'|'"));
	CHECK_EQ(type("x = abc", "'"), std::string("x = abc'|"));
	CHECK_EQ(type("", "\"\"\""), std::string("\"\"\"|\"\"\"")); /* triple */
}

TEST(backspace_pair) {
	CHECK_EQ(type("", "(\b"), std::string("|"));
	CHECK_EQ(type("", "'\b"), std::string("|"));
	CHECK_EQ(type("", "(x\b\b"), std::string("|"));
	CHECK_EQ(type("ab", "\b"), std::string("a|"));
}

TEST(wrap_selection) {
	Buffer b("x = a + b");
	b.set_auto_close(true);
	b.set_selection(4, 9);
	b.insert_char('(');
	CHECK_EQ(b.text(), std::string("x = (a + b)"));
	CHECK_EQ(b.selected_text(), std::string("a + b"));
}

TEST(undo_pair_is_one_step) {
	Buffer b("");
	b.set_auto_close(true);
	b.insert_char('(');
	CHECK(b.undo());
	CHECK_EQ(b.text(), std::string(""));
}

TEST(newline_moves_closer) {
	Buffer b("f()");
	b.set_caret(2);
	b.insert_newline("    ");
	CHECK_EQ(b.text(), std::string("f(\n    \n)"));
	CHECK_EQ(b.caret(), 7);
}
