// SPDX-License-Identifier: MIT
/* complete_test.cpp -- py_complete: document words + Python names. */

#include "test.h"

#include "py_complete.h"

using namespace py_code;

static std::string best(Completer &c, const std::string &text, int min_prefix = 3, int cycle = 0) {
	static int rev = 0;
	c.sync(text, ++rev);
	Completion out;
	return c.suggest(text, (int)text.size(), cycle, min_prefix, out) ? out.word : "<none>";
}

TEST(language_names) {
	Completer c;
	CHECK_EQ(best(c, "isins"), "isinstance");
	CHECK_EQ(best(c, "nonl"), "nonlocal");
	CHECK_EQ(best(c, "pr"), "<none>"); /* below the automatic threshold */
	CHECK_EQ(best(c, "pr", 1), "print");
}

TEST(document_words_rank_first) {
	Completer c;
	/* `printer` appears twice, so it beats the builtin `print`. */
	CHECK_EQ(best(c, "printer = 1\nprinter += 1\npri"), "printer");
}

TEST(token_and_span) {
	Completer c;
	const std::string text = "frequency = 2\nx = fre";
	c.sync(text, 1);
	Completion out;
	CHECK(c.suggest(text, (int)text.size(), 0, 3, out));
	CHECK_EQ(out.start, (int)text.size() - 3);
	CHECK_EQ(out.end, (int)text.size());
	CHECK_EQ(out.word, std::string("frequency"));
}

TEST(no_sigils) {
	Completer c;
	/* `~` is an operator in Python, not part of a name. */
	CHECK_EQ(best(c, "value = 1\n~val"), "value");
}

TEST(word_already_complete) {
	Completer c;
	CHECK_EQ(best(c, "return"), "<none>");
}
