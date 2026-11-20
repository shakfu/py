// SPDX-License-Identifier: MIT
/* completion.h -- completion popup model, independent of the item source. */

#pragma once

#include "core/completion_item.h"

#include <string>
#include <vector>

namespace pyc {

struct CompletionEdit {
	int start = -1; /* byte range to replace; -1 = nothing to accept */
	int end = -1;
	std::string text;
};

struct CompletionWindow {
	int first = 0;
	int count = 0;
};

class CompletionSession {
  public:
	/* Word start = start of the [A-Za-z0-9_] run ending at `caret`. Returns is_open(). */
	bool open(const std::string &text, int caret, std::vector<CompletionItem> items);

	/* Refilters; closes and returns false when the caret is before the word
	 * start, the line changed or nothing matches. */
	bool update(const std::string &text, int caret);

	bool is_open() const { return mOpen; }
	void close();

	int word_start() const { return mWordStart; }
	const std::string &prefix() const { return mPrefix; }
	int count() const { return (int)mShown.size(); }           /* filtered items */
	const CompletionItem &item(int i) const;                    /* i in [0, count) */
	int selected() const { return mSelected; }
	void select_next(); /* wraps */
	void select_prev();
	CompletionWindow visible(int rows); /* scrolls minimally to keep the selection shown */

	/* The selected item's edit; closes the session. */
	CompletionEdit accept();

  private:
	bool filter();

	bool mOpen = false;
	int mWordStart = 0;
	int mCaret = 0;
	std::string mPrefix;
	std::vector<CompletionItem> mItems;
	std::vector<int> mShown; /* indices into mItems */
	int mSelected = 0;
	int mFirst = 0;
};

} // namespace pyc
