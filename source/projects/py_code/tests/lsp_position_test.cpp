// SPDX-License-Identifier: MIT
#include "lsp/position.h"

#include "test.h"

using namespace pyc::lsp;

namespace {
/* "a" ASCII, "\xc3\xa4" 2 bytes, "\xe2\x82\xac" 3 bytes, "\xf0\x9f\x98\x80" 4 bytes. */
const std::string kText = "a\xc3\xa4\xe2\x82\xac\xf0\x9f\x98\x80z\nx\xf0\x9f\x98\x80\n";
} // namespace

TEST(position_table) {
	struct Row {
		int offset, line, u16, u8;
	};
	const Row rows[] = {
		{0, 0, 0, 0},   {1, 0, 1, 1},   {3, 0, 2, 3},  {6, 0, 3, 6},  {10, 0, 5, 10}, {11, 0, 6, 11},
		{12, 1, 0, 0},  {13, 1, 1, 1},  {17, 1, 3, 5}, {18, 2, 0, 0},
	};
	for (const Row &r : rows) {
		const Position p16 = to_position(kText, r.offset, true);
		const Position p8 = to_position(kText, r.offset, false);
		CHECK_EQ(p16.line, r.line);
		CHECK_EQ(p16.character, r.u16);
		CHECK_EQ(p8.line, r.line);
		CHECK_EQ(p8.character, r.u8);
		CHECK_EQ(to_offset(kText, r.line, r.u16, true), r.offset);
		CHECK_EQ(to_offset(kText, r.line, r.u8, false), r.offset);
	}
}

TEST(position_clamping) {
	CHECK_EQ(to_position(kText, -5, true).character, 0);
	const Position end = to_position(kText, 1000, true);
	CHECK_EQ(end.line, 2);
	CHECK_EQ(end.character, 0);
	CHECK_EQ(to_offset(kText, 0, 99, true), 11);   /* clamped before the \n */
	CHECK_EQ(to_offset(kText, 1, 99, false), 17);
	CHECK_EQ(to_offset(kText, 9, 0, true), int(kText.size()));
	CHECK_EQ(to_offset(kText, -1, 3, true), 0);
	CHECK_EQ(to_offset(kText, 0, -2, true), 0);
	CHECK_EQ(to_offset("", 0, 4, true), 0);
	CHECK_EQ(to_position("", 0, true).line, 0);
}

TEST(position_inside_code_point) {
	/* UTF-16 unit inside a surrogate pair snaps to the code point start. */
	CHECK_EQ(to_offset(kText, 0, 4, true), 6);
	/* UTF-8 byte inside a sequence snaps back too. */
	CHECK_EQ(to_offset(kText, 0, 2, false), 1);
	CHECK_EQ(to_offset(kText, 0, 8, false), 6);
	/* A byte offset inside a code point counts the units before it. */
	CHECK_EQ(to_position(kText, 2, true).character, 1);
	CHECK_EQ(to_position(kText, 8, true).character, 3);
}

TEST(position_invalid_utf8) {
	const std::string bad = "a\xff\xc3(\xe2\x82";
	CHECK_EQ(utf8_len(bad, 1), 1);
	CHECK_EQ(utf8_len(bad, 2), 1); /* lead without continuation */
	CHECK_EQ(utf8_len(bad, 4), 1); /* truncated sequence */
	CHECK_EQ(utf8_safe(bad), std::string("a?" "?(??"));
	CHECK_EQ(to_position(bad, 7, true).character, 6); /* clamped to the end */
	CHECK_EQ(to_offset(bad, 0, 3, true), 3);
	/* Overlong and surrogate encodings are invalid. */
	CHECK_EQ(utf8_safe("\xc0\x80\xed\xa0\x80"), std::string("?????"));
	CHECK_EQ(utf8_safe(kText), kText);
}

TEST(position_advance_units) {
	CHECK_EQ(advance_units(kText, 0, 5, true), 10);
	CHECK_EQ(advance_units(kText, 0, 7, true), 12); /* crosses the newline */
	CHECK_EQ(advance_units(kText, 1, 2, false), 3);
	CHECK_EQ(advance_units(kText, 0, 1000, true), int(kText.size()));
}

TEST(position_line_index_reuse) {
	const LineIndex idx(kText);
	for (int off = 0; off <= int(kText.size()); ++off) {
		const Position p = idx.position(off, true);
		const int back = idx.offset(p.line, p.character, true);
		CHECK(back <= off);
		CHECK(off - back < 4);
	}
}
