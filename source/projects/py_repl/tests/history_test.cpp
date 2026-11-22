// SPDX-License-Identifier: MIT
/* history_test.cpp -- pyc::History (py_repl-spec.md, 2.2). */

#include "core/history.h"
#include "test.h"

using namespace pyc;

TEST(history_add_skips_blank_and_repeat) {
	History h;
	h.add("a");
	h.add("a");
	h.add("   \n");
	h.add("");
	h.add("b");
	h.add("a");
	CHECK_EQ(h.size(), 3); /* a, b, a */
	CHECK_EQ(h.entries().back(), std::string("a"));
}

TEST(history_cap_drops_oldest) {
	History h(3);
	for (const char *s : {"1", "2", "3", "4", "5"})
		h.add(s);
	CHECK_EQ(h.size(), 3);
	CHECK_EQ(h.entries().front(), std::string("3"));
}

TEST(history_browse_back_and_forward_to_draft) {
	History h;
	h.add("a");
	h.add("b");
	std::string s;
	CHECK(h.prev("draft", &s));
	CHECK_EQ(s, std::string("b"));
	CHECK(h.browsing());
	CHECK_EQ(h.position(), 1);
	CHECK(h.prev("ignored", &s)); /* the draft is stored once */
	CHECK_EQ(s, std::string("a"));
	CHECK(!h.prev("ignored", &s)); /* at the oldest: stay */
	CHECK_EQ(h.position(), 0);
	CHECK(h.next(&s));
	CHECK_EQ(s, std::string("b"));
	CHECK(h.next(&s));
	CHECK_EQ(s, std::string("draft"));
	CHECK(!h.browsing());
	CHECK(!h.next(&s)); /* not browsing: stays */
}

TEST(history_empty_has_nothing_to_browse) {
	History h;
	std::string s = "unchanged";
	CHECK(!h.prev("draft", &s));
	CHECK(!h.browsing());
	CHECK_EQ(s, std::string("unchanged"));
}

TEST(history_add_and_reset_end_browsing) {
	History h;
	h.add("a");
	std::string s;
	h.prev("", &s);
	h.reset();
	CHECK(!h.browsing());
	h.prev("d1", &s);
	h.add("c");
	CHECK(!h.browsing());
	CHECK(h.prev("d2", &s)); /* a fresh browse starts at the newest */
	CHECK_EQ(s, std::string("c"));
}

TEST(history_load_applies_rules) {
	History h(2);
	h.add("old");
	h.load({"a", "a", "", "b", "c"});
	CHECK_EQ(h.size(), 2);
	CHECK_EQ(h.entries().front(), std::string("b"));
	CHECK_EQ(h.entries().back(), std::string("c"));
}

TEST(history_clear) {
	History h;
	h.add("a");
	std::string s;
	h.prev("", &s);
	h.clear();
	CHECK_EQ(h.size(), 0);
	CHECK(!h.browsing());
}
