// SPDX-License-Identifier: MIT
/* history.h -- submitted inputs and Up/Down browsing. Spec: py_repl-spec.md, 2.2. */

#pragma once

#include <string>
#include <vector>

namespace pyc {

class History {
  public:
	explicit History(int cap = 1000) : mCap(cap) {}

	/* Skips whitespace-only input and a repeat of the newest entry. Ends browsing. */
	void add(const std::string &code);

	/* The previous (older) entry; the first call stores `draft`. False at the
	 * oldest entry or with no entries: the input stays as it is. */
	bool prev(const std::string &draft, std::string *out);
	/* The next entry, then the stored draft (ending browsing). False when not browsing. */
	bool next(std::string *out);
	/* Ends browsing; call on any edit of the input. */
	void reset() { mBrowsing = false; }

	bool browsing() const { return mBrowsing; }
	int position() const { return mPos; } /* 0-based entry shown while browsing */
	int size() const { return (int)mEntries.size(); }

	const std::vector<std::string> &entries() const { return mEntries; } /* oldest first */
	/* Replaces the entries, keeping the newest `cap`. Ends browsing. */
	void load(const std::vector<std::string> &entries);
	void clear();

  private:
	std::vector<std::string> mEntries;
	std::string mDraft;
	int mPos = 0;
	bool mBrowsing = false;
	int mCap;
};

} // namespace pyc
