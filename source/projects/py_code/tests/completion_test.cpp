// SPDX-License-Identifier: MIT
/* completion_test.cpp -- CompletionSession: filtering, ordering, window, accept. */

#include "test.h"

#include "core/completion.h"

using namespace pyc;
using std::string;

static CompletionItem item(const string &label, const string &sort = "", const string &filter = "") {
	CompletionItem it;
	it.label = it.insert = label;
	it.sort = sort;
	it.filter = filter;
	return it;
}

static std::vector<CompletionItem> items(std::initializer_list<const char *> labels) {
	std::vector<CompletionItem> v;
	for (const char *l : labels)
		v.push_back(item(l));
	return v;
}

static string shown(const CompletionSession &s) {
	string o;
	for (int i = 0; i < s.count(); ++i)
		o += (i ? "," : "") + s.item(i).label;
	return o;
}

TEST(completion_open_word_start) {
	CompletionSession s;
	CHECK(s.open("x = pri", 7, items({"print"})));
	CHECK(s.is_open());
	CHECK_EQ(s.word_start(), 4);
	CHECK_EQ(s.prefix(), string("pri"));
	CHECK(s.open("a.b_1", 5, items({"b_10"})));
	CHECK_EQ(s.word_start(), 2);
	CHECK(s.open("os.", 3, items({"path"})));
	CHECK_EQ(s.word_start(), 3);
	CHECK_EQ(s.prefix(), string(""));
	CHECK(s.open("", 0, items({"x"})));
	CHECK(s.open("ab", 99, items({"abc"}))); /* caret clamped */
	CHECK_EQ(s.prefix(), string("ab"));
	CHECK(!s.open("zz", 2, items({"print"}))); /* nothing matches */
	CHECK(!s.is_open());
	CHECK(!s.open("x", 1, {}));
}

TEST(completion_filter_order) {
	CompletionSession s;
	s.open("pri", 3, items({"sprint", "print", "range", "pr_int", "PRIVATE", "Pri"}));
	/* prefix matches (case-insensitive) first, then subsequence; input order inside */
	CHECK_EQ(shown(s), string("print,PRIVATE,Pri,sprint,pr_int"));
	s.open("PrI", 3, items({"sprint", "print"}));
	CHECK_EQ(shown(s), string("print,sprint"));
	s.open("", 0, items({"b", "a", "c"}));
	CHECK_EQ(shown(s), string("b,a,c"));
}

TEST(completion_sort_keys) {
	CompletionSession s;
	std::vector<CompletionItem> v = {item("pz", "3"), item("pa", "1"), item("xpy", "0"), item("pb", "2"),
									 item("xpa", "9"), item("pq")};
	s.open("p", 1, v);
	/* within a class: items without a key keep their place first, then by key */
	CHECK_EQ(shown(s), string("pq,pa,pb,pz,xpy,xpa"));
	s.open("", 0, v); /* empty prefix: original order, keys ignored */
	CHECK_EQ(shown(s), string("pz,pa,xpy,pb,xpa,pq"));
	std::vector<CompletionItem> tie = {item("pb", "1"), item("pa", "1")};
	s.open("p", 1, tie);
	CHECK_EQ(shown(s), string("pb,pa")); /* equal keys: input order */
}

TEST(completion_filter_text) {
	CompletionSession s;
	s.open("ge", 2, {item("get(key)", "", "get"), item("x", "", "agex"), item("geometry")});
	CHECK_EQ(shown(s), string("get(key),geometry,x"));
}

TEST(completion_update) {
	CompletionSession s;
	const auto v = items({"print", "private", "len"});
	s.open("x = p", 5, v);
	CHECK_EQ(shown(s), string("print,private"));
	CHECK(s.update("x = pri", 7));
	CHECK(s.update("x = priv", 8));
	CHECK_EQ(shown(s), string("private"));
	CHECK(s.update("x = p", 5)); /* backspace widens */
	CHECK_EQ(shown(s), string("print,private"));
	CHECK(s.update("x = ", 4)); /* at the word start: empty prefix */
	CHECK_EQ(shown(s), string("print,private,len"));
	CHECK(!s.update("x =", 3)); /* before the word start */
	CHECK(!s.is_open());
	CHECK(!s.update("x = p", 5)); /* closed stays closed */

	s.open("x = p", 5, v);
	CHECK(!s.update("x = pz", 6)); /* nothing matches */
	s.open("x = p", 5, v);
	CHECK(!s.update("x = p\n", 6)); /* line changed */
	s.open("x = p", 5, v);
	CHECK(!s.update("x = p", 9)); /* caret past the text */
}

TEST(completion_selection_wraps) {
	CompletionSession s;
	s.open("", 0, items({"a", "b", "c"}));
	CHECK_EQ(s.selected(), 0);
	s.select_prev();
	CHECK_EQ(s.selected(), 2);
	s.select_next();
	CHECK_EQ(s.selected(), 0);
	s.select_next();
	s.select_next();
	CHECK_EQ(s.selected(), 2);
	s.update("", 0); /* refilter resets the selection */
	CHECK_EQ(s.selected(), 0);
	CompletionSession empty;
	empty.select_next();
	empty.select_prev();
	CHECK_EQ(empty.selected(), 0);
}

TEST(completion_visible_window) {
	std::vector<CompletionItem> v;
	for (int i = 0; i < 20; ++i)
		v.push_back(item("w" + std::to_string(i)));
	CompletionSession s;
	s.open("w", 1, v);
	CompletionWindow w = s.visible(8);
	CHECK_EQ(w.first, 0);
	CHECK_EQ(w.count, 8);
	for (int i = 0; i < 7; ++i)
		s.select_next();
	CHECK_EQ(s.visible(8).first, 0); /* selection 7: still in view */
	s.select_next();
	CHECK_EQ(s.visible(8).first, 1); /* minimal scroll */
	s.select_next();
	CHECK_EQ(s.visible(8).first, 2);
	for (int i = 0; i < 3; ++i)
		s.select_prev();
	CHECK_EQ(s.selected(), 6);
	CHECK_EQ(s.visible(8).first, 2); /* 6 is visible in [2, 10) */
	for (int i = 0; i < 5; ++i)
		s.select_prev();
	CHECK_EQ(s.selected(), 1);
	CHECK_EQ(s.visible(8).first, 1);
	s.select_prev();
	s.select_prev(); /* wraps to 19 */
	w = s.visible(8);
	CHECK_EQ(w.first, 12);
	CHECK_EQ(w.count, 8);
	s.select_next(); /* wraps to 0 */
	CHECK_EQ(s.visible(8).first, 0);

	s.open("", 0, items({"a", "b", "c"}));
	w = s.visible(8);
	CHECK_EQ(w.first, 0);
	CHECK_EQ(w.count, 3);
	CHECK_EQ(s.visible(0).count, 0);
}

TEST(completion_accept_word_range) {
	CompletionSession s;
	std::vector<CompletionItem> v = items({"print"});
	v[0].insert = "print(";
	s.open("x = pr", 6, v);
	const CompletionEdit e = s.accept();
	CHECK_EQ(e.start, 4);
	CHECK_EQ(e.end, 6);
	CHECK_EQ(e.text, string("print("));
	CHECK(!s.is_open()); /* accept closes */
	CHECK_EQ(s.accept().start, -1);

	CompletionItem no_insert;
	no_insert.label = "label";
	s.open("la", 2, {no_insert});
	CHECK_EQ(s.accept().text, string("label")); /* empty insert falls back to the label */

	s.open("x = ", 4, items({"a", "b"}));
	s.select_next();
	const CompletionEdit b = s.accept();
	CHECK_EQ(b.text, string("b"));
	CHECK_EQ(b.start, 4);
	CHECK_EQ(b.end, 4);
}

TEST(completion_accept_item_range) {
	CompletionSession s;
	CompletionItem it = item("os.path");
	it.replace_start = 2;
	it.replace_end = 5;
	s.open("x=os.pa", 7, {it}); /* filter "pa" is a subsequence of "os.path" */
	CHECK(s.is_open());
	CompletionEdit e = s.accept();
	CHECK_EQ(e.start, 2);
	CHECK_EQ(e.end, 7); /* extended to the caret */
	s.open("x=os.pa", 7, {it});
	s.update("x=os.p", 6);
	CHECK_EQ(s.accept().end, 6);
	it.replace_end = 9;
	s.open("x=os.pa", 7, {it});
	CHECK_EQ(s.accept().end, 9); /* the item's end when the caret is before it */
	CompletionItem start_only = item("pathx");
	start_only.replace_start = 3; /* end -1: not a range */
	s.open("x=pa", 4, {start_only});
	e = s.accept();
	CHECK_EQ(e.start, 2);
	CHECK_EQ(e.end, 4);
}

TEST(completion_close) {
	CompletionSession s;
	s.open("a", 1, items({"ab"}));
	s.close();
	CHECK(!s.is_open());
	CHECK_EQ(s.count(), 0);
	CHECK(!s.update("ab", 2));
}
