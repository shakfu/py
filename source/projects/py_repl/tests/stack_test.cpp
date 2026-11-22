// SPDX-License-Identifier: MIT
/* stack_test.cpp -- pyc::Stack and content_rows (py_repl-spec.md, 2.4). */

#include "core/editor.h"
#include "core/layout.h"
#include "core/stack.h"
#include "test.h"

#include <cmath>

using namespace pyc;

namespace test {
inline std::string show(const Place &p) { return std::to_string(p.part) + ":" + std::to_string(p.row); }
} // namespace test

static bool near(double a, double b) { return std::fabs(a - b) < 1e-9; }

static int rows_of(const char *text) {
	Editor ed(text);
	Layout l;
	l.update(ed, 0);
	return content_rows(ed, l);
}

TEST(content_rows_drops_final_empty_row) {
	CHECK_EQ(rows_of(""), 0);
	CHECK_EQ(rows_of("a"), 1);
	CHECK_EQ(rows_of("a\n"), 1);
	CHECK_EQ(rows_of("a\nb\n"), 2);
	CHECK_EQ(rows_of("a\n\n"), 2); /* a blank line before the last newline stays */
	CHECK_EQ(rows_of("\n"), 1);
}

TEST(stack_place_and_row_of) {
	Stack s;
	s.update(3, 2);
	CHECK_EQ(s.rows(), 5);
	CHECK_EQ(s.place(0), (Place{0, 0}));
	CHECK_EQ(s.place(2), (Place{0, 2}));
	CHECK_EQ(s.place(3), (Place{1, 0}));
	CHECK_EQ(s.place(4), (Place{1, 1}));
	CHECK_EQ(s.place(99), (Place{1, 1})); /* clamped */
	CHECK_EQ(s.place(-1), (Place{0, 0}));
	for (int r = 0; r < s.rows(); ++r) {
		const Place p = s.place(r);
		CHECK_EQ(s.row_of(p.part, p.row), r);
	}
}

TEST(stack_without_transcript) {
	Stack s;
	s.update(0, 1);
	CHECK_EQ(s.place(0), (Place{1, 0}));
}

/* View 100, line 10, pad 5: 20 rows give content 210 and max scroll 110. */
TEST(stack_sticks_to_bottom) {
	Stack s;
	s.scroll().set_view(100, 10, 5);
	s.update(18, 2);
	CHECK(s.at_bottom());
	CHECK(near(s.scroll().scroll_y(), 110));
	s.update(28, 2); /* output arrived: still showing the last row */
	CHECK(near(s.scroll().scroll_y(), 210));
	CHECK(s.at_bottom());
}

TEST(stack_keeps_position_when_scrolled_up) {
	Stack s;
	s.scroll().set_view(100, 10, 5);
	s.update(18, 2);
	s.scroll().scroll_by(-50);
	CHECK(!s.at_bottom());
	s.update(28, 2);
	CHECK(near(s.scroll().scroll_y(), 60));
	CHECK(!s.at_bottom());
}

TEST(stack_clamps_when_rows_shrink) {
	Stack s;
	s.scroll().set_view(100, 10, 5);
	s.update(30, 2);
	s.scroll().scroll_by(-100);
	s.update(2, 1); /* transcript cleared */
	CHECK(near(s.scroll().scroll_y(), 0));
	CHECK(s.at_bottom()); /* everything fits */
}

TEST(stack_short_content_is_at_bottom) {
	Stack s;
	s.scroll().set_view(100, 10, 5);
	s.update(1, 1);
	CHECK(s.at_bottom());
	CHECK(near(s.scroll().scroll_y(), 0));
	CHECK(!s.scroll().thumb(100).visible);
}
