// SPDX-License-Identifier: MIT
/* layout_test.cpp -- pyc::Layout: wrap, row lookup, scroll, scroll bar. */

#include "test.h"

#include "core/editor.h"
#include "core/layout.h"

#include <cmath>

using namespace pyc;
using std::string;

/* Rows as "line:begin-end" joined by spaces. */
static string rows(const string &text, int cols) {
	Editor e(text);
	Layout l;
	l.update(e, cols);
	string o;
	for (int r = 0; r < l.row_count(); ++r) {
		const Row row = l.row(r);
		o += (r ? " " : "") + std::to_string(row.line) + ":" + std::to_string(row.cell_begin) + "-" +
			 std::to_string(row.cell_end);
	}
	return o;
}

static bool near(double a, double b) { return std::fabs(a - b) < 1e-6; }

TEST(layout_no_wrap) {
	CHECK_EQ(rows("", 10), string("0:0-0"));
	CHECK_EQ(rows("", 0), string("0:0-0"));
	CHECK_EQ(rows("abc\n\nde", 0), string("0:0-3 1:0-0 2:0-2"));
	CHECK_EQ(rows(string(50, 'x'), -1), string("0:0-50"));
	CHECK_EQ(rows("abc\n", 10), string("0:0-3 1:0-0"));
}

TEST(layout_wrap_rule) {
	struct {
		const char *text;
		int cols;
		const char *want;
	} cases[] = {
		{"aaaaaaaaaa", 10, "0:0-10"},                   /* exactly cols: one row */
		{"aaaaaaaaaab", 10, "0:0-10 0:10-11"},          /* hard break */
		{"aaaa bbbbbbbbb", 10, "0:0-5 0:5-14"},         /* after the last space */
		{"aaaa bbbb cccc", 10, "0:0-10 0:10-14"},       /* space at cell 9 */
		{"aaaa bbbb ccccdddd", 10, "0:0-10 0:10-18"},   /* space at 9: break after it */
		{"a bbbbbbbbbbbb", 10, "0:0-10 0:10-14"},       /* space in the first half: hard */
		{"aaaa,bbbbbbbbb", 10, "0:0-5 0:5-14"},
		{"aaaa;bbbbbbbbb", 10, "0:0-5 0:5-14"},
		{"aaaa(bbbbbbbbb", 10, "0:0-5 0:5-14"},
		{"aaaa[bbbbbbbbb", 10, "0:0-5 0:5-14"},
		{"aaaa{bbbbbbbbb", 10, "0:0-5 0:5-14"},
		{"aaaa)bbbbbbbbb", 10, "0:0-10 0:10-14"},       /* closers do not break */
		{"aaaa.bbbbbbbbb", 10, "0:0-10 0:10-14"},
		{"aaa, bbbbbbbbb", 10, "0:0-5 0:5-14"},         /* the last candidate wins */
		{"f(aaaa,bbbbbbbb)", 8, "0:0-7 0:7-15 0:15-16"},
		{"a bcdefgh", 5, "0:0-2 0:2-7 0:7-9"},          /* break column 2 >= 5/2 */
		{"a bcdefgh", 6, "0:0-6 0:6-9"},                /* break column 2 < 6/2 */
		{"abcdef", 1, "0:0-1 0:1-2 0:2-3 0:3-4 0:4-5 0:5-6"},
		{"ab\n\ncdefg", 3, "0:0-2 1:0-0 2:0-3 2:3-5"},
		{"\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9\xC3\xA9", 2, "0:0-2 0:2-4 0:4-5"}, /* cells, not bytes */
	};
	for (auto &c : cases)
		CHECK_EQ(rows(c.text, c.cols), string(c.want));
}

TEST(layout_row_of) {
	Editor e("aaaa bbbbbbbbb\nxy");
	Layout l;
	l.update(e, 10);
	CHECK_EQ(l.row_count(), 3);
	struct {
		int off, row;
	} cases[] = {{0, 0}, {4, 0}, {5, 1}, {13, 1}, {14, 1}, {15, 2}, {17, 2}, {-3, 0}, {99, 2}};
	for (auto &c : cases)
		CHECK_EQ(l.row_of(c.off), c.row);

	Editor u("\xC3\xA9\xC3\xA9\xC3\xA9");
	l.update(u, 2);
	CHECK_EQ(l.row_of(2), 0);
	CHECK_EQ(l.row_of(4), 1); /* break offset: later row */
	CHECK_EQ(l.row_of(6), 1); /* line end: last row */
}

TEST(layout_offset_at) {
	Editor e("aaaa bbbbbbbbb\nxy");
	Layout l;
	l.update(e, 10);
	struct {
		int row, cell, off;
	} cases[] = {
		{0, 0, 0}, {0, 3, 3}, {0, 4, 4}, {0, 5, 4}, {0, 99, 4}, /* before the break */
		{0, -3, 0}, {1, 0, 5}, {1, 9, 14}, {1, 99, 14},          /* last row: line end */
		{2, 1, 16}, {2, 9, 17}, {9, 0, 15}, {-1, 2, 2},
	};
	for (auto &c : cases)
		CHECK_EQ(l.offset_at(c.row, c.cell), c.off);

	Editor u("\xC3\xA9\xC3\xA9\xC3\xA9");
	l.update(u, 2);
	CHECK_EQ(l.offset_at(0, 1), 2);
	CHECK_EQ(l.offset_at(0, 5), 2);
	CHECK_EQ(l.offset_at(1, 0), 4);
	CHECK_EQ(l.offset_at(1, 5), 6);
	CHECK_EQ(l.cell_in_row(4), 0);
	CHECK_EQ(l.cell_in_row(6), 1);
	CHECK_EQ(l.cell_in_row(2), 1);
}

TEST(layout_rebuilds_on_change) {
	Editor e("aaaa bbbb cccc");
	Layout l;
	l.update(e, 10);
	CHECK_EQ(l.row_count(), 2);
	l.update(e, 0);
	CHECK_EQ(l.row_count(), 1);
	l.update(e, 5);
	CHECK_EQ(l.row_count(), 3);
	e.set_caret(e.length());
	e.insert("\nz");
	l.update(e, 5);
	CHECK_EQ(l.row_count(), 4);
	e.reset("");
	l.update(e, 5);
	CHECK_EQ(l.row_count(), 1);
	Editor other("a\nb\nc");
	l.update(other, 5); /* another editor at its own revision */
	CHECK_EQ(l.row_count(), 3);
}

/* 20 rows, view 100, line 10, pad 5: content 210, max scroll 110. */
static void twenty(Editor &e, Layout &l) {
	string t;
	for (int i = 0; i < 20; ++i)
		t += i ? "\nx" : "x";
	e.reset(t);
	l.update(e, 0);
	l.set_view(100, 10, 5);
}

TEST(layout_scroll_clamp) {
	Editor e;
	Layout l;
	twenty(e, l);
	CHECK(near(l.content_h(), 210));
	CHECK(near(l.max_scroll(), 110));
	l.scroll_by(1000);
	CHECK(near(l.scroll_y(), 110));
	l.scroll_by(-30);
	CHECK(near(l.scroll_y(), 80));
	l.scroll_by(-1000);
	CHECK(near(l.scroll_y(), 0));
	l.set_scroll(500); /* restore: unclamped until clamp() */
	CHECK(near(l.scroll_y(), 500));
	l.clamp();
	CHECK(near(l.scroll_y(), 110));
	l.set_scroll(-3);
	l.clamp();
	CHECK(near(l.scroll_y(), 0));

	Editor small("a\nb");
	l.update(small, 0);
	l.set_scroll(40);
	l.clamp(); /* content 30 < view 100 */
	CHECK(near(l.scroll_y(), 0));
	l.scroll_by(10);
	CHECK(near(l.scroll_y(), 0));
}

TEST(layout_rows_and_y) {
	Editor e;
	Layout l;
	twenty(e, l);
	CHECK(near(l.row_y(0), 5));
	CHECK(near(l.row_y(3), 35));
	l.set_scroll(20);
	CHECK(near(l.row_y(3), 15));
	l.set_scroll(0);
	struct {
		double y;
		int row;
	} cases[] = {{0, 0}, {4.9, 0}, {14.9, 0}, {15, 1}, {95, 9}, {-50, 0}, {10000, 19}};
	for (auto &c : cases)
		CHECK_EQ(l.row_at_y(c.y), c.row);
	CHECK_EQ(l.first_visible_row(), 0);
	CHECK_EQ(l.last_visible_row(), 9); /* row 9 is partly visible */
	l.set_scroll(110);
	CHECK_EQ(l.first_visible_row(), 10);
	CHECK_EQ(l.last_visible_row(), 19);
	l.set_scroll(5);
	CHECK_EQ(l.first_visible_row(), 0);
	CHECK_EQ(l.last_visible_row(), 9);
	l.set_scroll(6);
	CHECK_EQ(l.last_visible_row(), 10);
	CHECK_EQ(l.row_at_y(20), 2);
}

TEST(layout_ensure_row_visible) {
	Editor e;
	Layout l;
	twenty(e, l);
	l.ensure_row_visible(15); /* bottom of row 15 + pad at the view bottom */
	CHECK(near(l.scroll_y(), 70));
	CHECK(near(l.row_y(15) + 10 + 5, 100));
	l.ensure_row_visible(15); /* already visible: no scroll */
	CHECK(near(l.scroll_y(), 70));
	l.ensure_row_visible(9);
	CHECK(near(l.scroll_y(), 70));
	l.ensure_row_visible(2);
	CHECK(near(l.scroll_y(), 20));
	CHECK(near(l.row_y(2), 5));
	l.ensure_row_visible(0);
	CHECK(near(l.scroll_y(), 0));
	l.ensure_row_visible(19);
	CHECK(near(l.scroll_y(), 110));
	l.ensure_row_visible(99); /* clamped to the last row */
	CHECK(near(l.scroll_y(), 110));
	l.ensure_row_visible(-4);
	CHECK(near(l.scroll_y(), 0));
	l.set_view(12, 10, 5); /* view smaller than row + pads: the top wins */
	l.ensure_row_visible(5);
	CHECK(near(l.scroll_y(), 50));
}

TEST(layout_page_rows) {
	Layout l;
	struct {
		double view, line;
		int want;
	} cases[] = {{100, 10, 9}, {105, 10, 9}, {99, 10, 8}, {25, 10, 1}, {5, 10, 1}, {0, 10, 1}, {100, 0, 1}};
	for (auto &c : cases) {
		l.set_view(c.view, c.line, 4);
		CHECK_EQ(l.page_rows(), c.want);
	}
}

TEST(layout_thumb) {
	Editor e;
	Layout l;
	twenty(e, l);
	Thumb t = l.thumb(100);
	CHECK(t.visible);
	CHECK(near(t.h, 100.0 * 100 / 210));
	CHECK(near(t.y, 0));
	l.set_scroll(110);
	t = l.thumb(100);
	CHECK(near(t.y, 100 - t.h));
	l.set_scroll(55);
	t = l.thumb(100);
	CHECK(near(t.y, (100 - t.h) / 2));

	Editor small("a");
	l.update(small, 0);
	CHECK(!l.thumb(100).visible);
	l.set_view(30, 10, 5);
	CHECK(!l.thumb(100).visible); /* content 20 < view 30 */
	l.set_view(20, 10, 5);
	CHECK(!l.thumb(100).visible); /* content equals view */

	string many;
	for (int i = 0; i < 1000; ++i)
		many += "x\n";
	Editor big(many);
	l.update(big, 0);
	l.set_view(100, 10, 5);
	l.set_scroll(0);
	CHECK(near(l.thumb(100).h, Layout::kThumbMin)); /* minimum height */
	CHECK(near(l.thumb(10).h, 10));                 /* never taller than the track */
}

TEST(layout_drag_thumb) {
	Editor e;
	Layout l;
	twenty(e, l);
	const double h = l.thumb(100).h;
	l.drag_thumb((100 - h) / 2, 100);
	CHECK(near(l.scroll_y(), 55));
	l.drag_thumb(100 - h, 100);
	CHECK(near(l.scroll_y(), 110));
	l.drag_thumb(500, 100);
	CHECK(near(l.scroll_y(), 110));
	l.drag_thumb(-20, 100);
	CHECK(near(l.scroll_y(), 0));
	l.drag_thumb(l.thumb(100).y, 100); /* round trip */
	CHECK(near(l.scroll_y(), 0));
	l.set_scroll(37);
	l.drag_thumb(l.thumb(100).y, 100);
	CHECK(near(l.scroll_y(), 37));
}
