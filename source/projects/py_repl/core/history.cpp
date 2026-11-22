// SPDX-License-Identifier: MIT
/* history.cpp -- see history.h. */

#include "core/history.h"

namespace pyc {

static bool blank(const std::string &s) { return s.find_first_not_of(" \t\r\n") == std::string::npos; }

void History::add(const std::string &code) {
	mBrowsing = false;
	if (blank(code) || (!mEntries.empty() && mEntries.back() == code))
		return;
	mEntries.push_back(code);
	if (mCap > 0 && (int)mEntries.size() > mCap)
		mEntries.erase(mEntries.begin(), mEntries.end() - mCap);
}

bool History::prev(const std::string &draft, std::string *out) {
	if (!mBrowsing) {
		if (mEntries.empty())
			return false;
		mDraft = draft;
		mPos = (int)mEntries.size();
		mBrowsing = true;
	}
	if (mPos == 0)
		return false;
	*out = mEntries[(size_t)--mPos];
	return true;
}

bool History::next(std::string *out) {
	if (!mBrowsing)
		return false;
	if (mPos + 1 < (int)mEntries.size()) {
		*out = mEntries[(size_t)++mPos];
		return true;
	}
	*out = mDraft;
	mBrowsing = false;
	return true;
}

void History::load(const std::vector<std::string> &entries) {
	mEntries.clear();
	for (const std::string &e : entries)
		add(e);
}

void History::clear() {
	mEntries.clear();
	mBrowsing = false;
}

} // namespace pyc
