// SPDX-License-Identifier: MIT
/* submit_test.cpp -- pyc::return_action (py_repl-spec.md, 2.3). */

#include "core/editor.h"
#include "core/submit.h"
#include "test.h"

using namespace pyc;

namespace test {
inline std::string show(const ReturnAction &a) { return a == ReturnAction::Submit ? "submit" : "newline"; }
} // namespace test

struct Case {
	const char *text;
	int caret; /* -1: end of text */
	Completeness c;
	bool force;
	ReturnAction want;
};

TEST(return_action_rules) {
	const auto S = ReturnAction::Submit, N = ReturnAction::Newline;
	const auto C = Completeness::Complete, I = Completeness::Incomplete;
	const auto V = Completeness::Invalid, U = Completeness::Unknown;
	const Case cases[] = {
		{"x = 1", -1, C, false, S},
		{"def f():", -1, I, false, N},
		{"1 +* 2", -1, V, false, S},       /* submit to show the error */
		{"x", -1, U, false, S},            /* unlinked: submit to show why */
		{"", -1, C, false, S},             /* empty: submit does nothing */
		{"x = 1", 2, C, false, N},         /* caret inside the code */
		{"x = 1", 4, C, false, N},         /* before the last character */
		{"x = 1   ", 5, C, false, S},      /* only whitespace after the caret */
		{"x = 1\n    ", 6, C, false, S},   /* blank last line after the caret */
		{"def f():", 2, I, true, S},       /* Cmd+Return */
		{"def f():\n    pass", -1, I, true, S},
	};
	for (const Case &k : cases) {
		Editor ed(k.text);
		ed.set_caret(k.caret < 0 ? ed.length() : k.caret);
		CHECK_EQ(return_action(ed, k.c, k.force), k.want);
	}
}
