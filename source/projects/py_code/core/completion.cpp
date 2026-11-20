// SPDX-License-Identifier: MIT
/* completion.cpp -- see completion.h. */

#include "core/completion.h"

#include <algorithm>

namespace pyc {

static bool is_word(char c) {
	return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static char lower(char c) { return c >= 'A' && c <= 'Z' ? (char)(c - 'A' + 'a') : c; }

static bool starts_with_ci(const std::string &s, const std::string &p) {
	if (p.size() > s.size())
		return false;
	for (size_t i = 0; i < p.size(); ++i)
		if (lower(s[i]) != lower(p[i]))
			return false;
	return true;
}

static bool subsequence_ci(const std::string &s, const std::string &p) {
	size_t j = 0;
	for (size_t i = 0; i < s.size() && j < p.size(); ++i)
		if (lower(s[i]) == lower(p[j]))
			++j;
	return j == p.size();
}

bool CompletionSession::open(const std::string &text, int caret, std::vector<CompletionItem> items) {
	caret = std::max(0, std::min(caret, (int)text.size()));
	int ws = caret;
	while (ws > 0 && is_word(text[(size_t)ws - 1]))
		--ws;
	mWordStart = ws;
	mItems = std::move(items);
	mOpen = true;
	return update(text, caret);
}

bool CompletionSession::update(const std::string &text, int caret) {
	if (!mOpen)
		return false;
	if (caret < mWordStart || caret > (int)text.size() ||
		text.find('\n', (size_t)mWordStart) < (size_t)caret) {
		close();
		return false;
	}
	mCaret = caret;
	mPrefix = text.substr((size_t)mWordStart, (size_t)(caret - mWordStart));
	if (!filter()) {
		close();
		return false;
	}
	return true;
}

bool CompletionSession::filter() {
	std::vector<int> pre, sub;
	for (int i = 0; i < (int)mItems.size(); ++i) {
		const CompletionItem &it = mItems[(size_t)i];
		const std::string &f = it.filter.empty() ? it.label : it.filter;
		if (starts_with_ci(f, mPrefix))
			pre.push_back(i);
		else if (subsequence_ci(f, mPrefix))
			sub.push_back(i);
	}
	if (!mPrefix.empty()) {
		/* Sort key first (items without one keep their place ahead), then input order. */
		auto by_sort = [&](int a, int b) {
			const std::string &sa = mItems[(size_t)a].sort, &sb = mItems[(size_t)b].sort;
			return sa != sb ? sa < sb : a < b;
		};
		std::sort(pre.begin(), pre.end(), by_sort);
		std::sort(sub.begin(), sub.end(), by_sort);
	}
	mShown = pre;
	mShown.insert(mShown.end(), sub.begin(), sub.end());
	mSelected = 0;
	mFirst = 0;
	return !mShown.empty();
}

void CompletionSession::close() {
	mOpen = false;
	mItems.clear();
	mShown.clear();
	mPrefix.clear();
	mSelected = mFirst = 0;
}

const CompletionItem &CompletionSession::item(int i) const {
	return mItems[(size_t)mShown[(size_t)std::max(0, std::min(i, count() - 1))]];
}

void CompletionSession::select_next() {
	if (count() > 0)
		mSelected = (mSelected + 1) % count();
}

void CompletionSession::select_prev() {
	if (count() > 0)
		mSelected = (mSelected + count() - 1) % count();
}

CompletionWindow CompletionSession::visible(int rows) {
	CompletionWindow w;
	const int n = count();
	if (rows <= 0 || n == 0)
		return w;
	if (mSelected < mFirst)
		mFirst = mSelected;
	if (mSelected >= mFirst + rows)
		mFirst = mSelected - rows + 1;
	mFirst = std::max(0, std::min(mFirst, n - rows));
	w.first = mFirst;
	w.count = std::min(rows, n - mFirst);
	return w;
}

CompletionEdit CompletionSession::accept() {
	CompletionEdit e;
	if (!mOpen || mShown.empty())
		return e;
	const CompletionItem &it = item(mSelected);
	e.text = it.insert.empty() ? it.label : it.insert;
	if (it.replace_start >= 0 && it.replace_end >= it.replace_start) {
		e.start = it.replace_start;
		e.end = std::max(it.replace_end, mCaret);
	} else {
		e.start = mWordStart;
		e.end = mCaret;
	}
	close();
	return e;
}

} // namespace pyc
