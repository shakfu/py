// SPDX-License-Identifier: MIT
/* words_test.cpp -- local completion source: scan, ranking, kinds. */

#include "test.h"

#include "core/words.h"

using namespace pyc;
using std::string;

static string labels(const std::vector<CompletionItem> &v) {
	string o;
	for (const CompletionItem &it : v)
		o += (o.empty() ? "" : ",") + it.label;
	return o;
}

TEST(words_scan) {
	Words w;
	w.sync("foo = foo + bar_1\n_x = 1abc + 0x1f + a.b  # foo\n", 1);
	CHECK_EQ(w.count("foo"), 3); /* comments count: plain text scan */
	CHECK_EQ(w.count("bar_1"), 1);
	CHECK_EQ(w.count("_x"), 1);
	CHECK_EQ(w.count("abc"), 0); /* part of a number */
	CHECK_EQ(w.count("x1f"), 0);
	CHECK_EQ(w.count("a"), 1);
	CHECK_EQ(w.count("b"), 1);
	CHECK_EQ(w.count("1abc"), 0);
	Words u;
	u.sync("caf\xC3\xA9 x\xC3\xA9y", 1);
	CHECK_EQ(u.count("caf"), 1); /* identifiers are ASCII */
	CHECK_EQ(u.count("y"), 1);
}

TEST(words_sync_by_revision) {
	Words w;
	w.sync("alpha", 1);
	w.sync("alpha beta", 1); /* same revision: ignored */
	CHECK_EQ(w.scans(), 1);
	CHECK_EQ(w.count("beta"), 0);
	w.sync("beta", 2);
	CHECK_EQ(w.scans(), 2);
	CHECK_EQ(w.count("beta"), 1);
	CHECK_EQ(w.count("alpha"), 0); /* vanished words go */
	Words z;
	z.sync("q", 0); /* first sync always scans, even at revision 0 */
	CHECK_EQ(z.count("q"), 1);
}

TEST(words_ranking) {
	Words w;
	w.sync("alpha alpha alphabet al alp alq", 1);
	/* freq desc, length asc, name; `al` itself excluded; builtin `all` has freq 0 */
	CHECK_EQ(labels(w.query("al", 0)), string("alpha,alp,alq,alphabet,all"));
	CHECK_EQ(labels(w.query("al", 2)), string("alpha,alp"));
	CHECK_EQ(labels(w.query("Al", 0)), string("")); /* case-sensitive */
	CHECK_EQ(labels(w.query("alphabet", 0)), string(""));
	CHECK_EQ(labels(w.query("zz", 0)), string(""));
}

TEST(words_builtins_keywords) {
	Words w;
	w.sync("", 1);
	CHECK_EQ(labels(w.query("whi", 0)), string("while"));
	CHECK_EQ(labels(w.query("isi", 0)), string("isinstance"));
	CHECK_EQ(labels(w.query("Tr", 0)), string("True"));
	CHECK_EQ(labels(w.query("ValueE", 0)), string("ValueError"));
	CHECK_EQ(labels(w.query("pri", 0)), string("print"));
	const auto ret = w.query("retu", 0);
	CHECK_EQ(ret.size(), (size_t)1);
	CHECK_EQ(ret[0].kind, 14);
	CHECK_EQ(w.query("pri", 0)[0].kind, 3);
	CHECK_EQ(w.count("print"), 0);
	CHECK((int)w.query("", 0).size() >= (int)(python_keywords().size() + python_builtins().size()));
}

TEST(words_items) {
	Words w;
	w.sync("printer = 1\nprint(printer)\nwhile_ = 2\nwhile True: pass\n", 1);
	const auto items = w.query("pri", 0);
	CHECK_EQ(labels(items), string("printer,print"));
	CHECK_EQ(items[0].kind, 1); /* document word */
	CHECK_EQ(items[1].kind, 3); /* builtin, counted in the document */
	for (const CompletionItem &it : items) {
		CHECK_EQ(it.insert, it.label);
		CHECK_EQ(it.replace_start, -1);
		CHECK_EQ(it.replace_end, -1);
	}
	const auto wh = w.query("whi", 0);
	CHECK_EQ(labels(wh), string("while,while_"));
	CHECK_EQ(wh[0].kind, 14);
	CHECK_EQ(wh[1].kind, 1);
}
