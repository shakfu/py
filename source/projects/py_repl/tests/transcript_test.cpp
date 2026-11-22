// SPDX-License-Identifier: MIT
/* transcript_test.cpp -- pyc::Transcript (py_repl-spec.md, 2.1). */

#include "core/transcript.h"
#include "test.h"

using namespace pyc;

namespace test {
inline std::string show(const Kind &k) { return kind_name(k); }
} // namespace test

/* Spans cover the text exactly, in order. */
static bool covers(const Transcript &t) {
	int at = 0;
	for (const Span &s : t.spans()) {
		if (s.start != at || s.end <= s.start)
			return false;
		at = s.end;
	}
	return at == (int)t.text().size();
}

static std::string span_text(const Transcript &t, size_t i) {
	const Span &s = t.spans()[i];
	return t.text().substr((size_t)s.start, (size_t)(s.end - s.start));
}

TEST(transcript_input_ends_with_newline) {
	Transcript t;
	t.append_input("x = 1");
	CHECK_EQ(t.text(), std::string("x = 1\n"));
	CHECK_EQ(t.spans().size(), (size_t)1);
	CHECK_EQ(t.spans()[0].kind, Kind::Input);
	t.append_input("y = 2\n");
	CHECK_EQ(t.text(), std::string("x = 1\ny = 2\n")); /* no doubled newline */
	CHECK_EQ(t.spans().size(), (size_t)2);              /* inputs never join */
	CHECK(covers(t));
}

TEST(transcript_output_joins_same_kind) {
	Transcript t;
	t.append_input("print(1); print(2)");
	t.append(Kind::Out, "1");
	t.append(Kind::Out, "\n2\n");
	t.append(Kind::Err, "e\n");
	t.append(Kind::Out, "3\n");
	CHECK_EQ(t.spans().size(), (size_t)4); /* input, out, err, out */
	CHECK_EQ(span_text(t, 1), std::string("1\n2\n"));
	CHECK_EQ(t.spans()[2].kind, Kind::Err);
	CHECK(covers(t));
}

TEST(transcript_input_starts_on_new_line) {
	Transcript t;
	t.append(Kind::Out, "no newline");
	t.append_input("x");
	CHECK_EQ(t.text(), std::string("no newline\nx\n"));
	CHECK_EQ(span_text(t, 0), std::string("no newline\n")); /* the newline joins the output */
	CHECK(covers(t));
}

TEST(transcript_end_run_and_info) {
	Transcript t;
	t.end_run();
	CHECK_EQ(t.text(), std::string("")); /* nothing to end */
	t.append(Kind::Result, "42");
	t.end_run();
	t.end_run();
	CHECK_EQ(t.text(), std::string("42\n"));
	t.info("py 'foo' not found");
	CHECK_EQ(t.text(), std::string("42\npy 'foo' not found\n"));
	CHECK_EQ(t.spans().back().kind, Kind::Info);
	CHECK(covers(t));
}

TEST(transcript_empty_append_is_ignored) {
	Transcript t;
	const int r = t.revision();
	t.append(Kind::Out, "");
	CHECK_EQ(t.revision(), r);
	CHECK(t.spans().empty());
}

TEST(transcript_revision_and_clear) {
	Transcript t;
	const int r0 = t.revision();
	t.append(Kind::Out, "a\n");
	CHECK(t.revision() > r0);
	const int r1 = t.revision();
	t.clear();
	CHECK(t.revision() > r1);
	CHECK_EQ(t.text(), std::string(""));
	CHECK(t.spans().empty());
	CHECK_EQ(t.newline_count(), 0);
}

TEST(transcript_trims_whole_lines_past_a_quarter) {
	Transcript t(8);
	for (int i = 0; i < 10; ++i) /* 10 lines: within 8 + 8/4 */
		t.append(Kind::Out, std::to_string(i) + "\n");
	CHECK_EQ(t.newline_count(), 10);
	t.append_input("x"); /* 11 lines: over, trim to 8 */
	CHECK_EQ(t.newline_count(), 8);
	CHECK_EQ(t.text(), std::string("3\n4\n5\n6\n7\n8\n9\nx\n"));
	CHECK_EQ(t.spans().size(), (size_t)2); /* the out span, trimmed, and the input */
	CHECK_EQ(t.spans()[0].start, 0);
	CHECK(covers(t));
}

TEST(transcript_trim_drops_spans_entirely_before_the_cut) {
	Transcript t(4);
	t.append_input("a");
	t.append(Kind::Result, "1\n");
	t.append_input("b");
	t.append(Kind::Result, "2\n");
	t.append_input("c"); /* 5 lines: within 4 + 1 */
	CHECK_EQ(t.spans().size(), (size_t)5);
	t.append(Kind::Result, "3\n"); /* 6 lines: drop "a", "1" */
	CHECK_EQ(t.text(), std::string("b\n2\nc\n3\n"));
	CHECK_EQ(t.spans().size(), (size_t)4);
	CHECK_EQ(t.spans()[0].kind, Kind::Input);
	CHECK(covers(t));
}

TEST(transcript_set_max_lines_trims_now) {
	Transcript t(100);
	for (int i = 0; i < 20; ++i)
		t.append(Kind::Out, "l\n");
	t.set_max_lines(4);
	CHECK_EQ(t.newline_count(), 4);
	CHECK(covers(t));
}

TEST(transcript_append_input_kind_routes_to_append_input) {
	Transcript t;
	t.append(Kind::Out, "o");
	t.append(Kind::Input, "x");
	CHECK_EQ(t.text(), std::string("o\nx\n"));
	CHECK_EQ(t.spans().back().kind, Kind::Input);
}
