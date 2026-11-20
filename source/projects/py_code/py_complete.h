// SPDX-License-Identifier: GPL-3.0-only
/* py_complete.h -- completion candidates: `text + caret -> one candidate`.
 *
 * Sources: the document's identifiers (ranked by frequency), then Python
 * keywords and builtins (frequency 0). Ranking is
 * deterministic: frequency desc, shorter, alphabetical. The word already typed
 * is never suggested. `min_prefix` 0 accepts an empty prefix. The document scan
 * is cached by buffer revision. */

#pragma once

#include <string>
#include <vector>

namespace py_code {

/* One concrete suggestion: replace [start, end) with `word`. `end` is always the
 * caret; the text to the right of the caret is never touched. */
struct Completion {
	bool valid;
	int start;        /* first byte of the token (includes `~`/`\` when there is one) */
	int end;          /* end of the token == caret */
	std::string word; /* the whole candidate */
	int index;        /* position of this one among the alternatives (0 = the best) */
	int alternatives; /* how many alternatives this prefix has */

	Completion() : valid(false), start(0), end(0), index(0), alternatives(0) {}
};

class Completer {
  public:
	Completer();

	/* Re-reads the document if it changed (`revision` != the last one seen). Cheap to
	 * call on every key: if the revision did not change it does nothing. */
	void sync(const std::string &text, int revision);

	/* `cycle` skips alternatives (0, 1, 2... and wraps). `min_prefix` is the
	 * minimum token length in bytes; 0 accepts an empty prefix. */
	bool suggest(const std::string &text, int caret, int cycle, int min_prefix, Completion &out) const;

	/* Identifier byte: [A-Za-z0-9_] (the same ASCII table as
	 * `Buffer::is_word_byte`; repeated here so the module does not depend on the
	 * buffer -- it only sees `std::string`). */
	static bool is_word_byte(unsigned char c);

	/* Test instrumentation (not used by the object). */
	int rebuilds() const { return mRebuilds; }
	int name_count() const { return (int)mNames.size(); }

  private:
	struct Entry {
		std::string word;
		int freq;
	};

	/* Candidates sorted by `word` (prefix search with lower_bound). */
	std::vector<Entry> mNames; /* document identifiers + language names */
	int mRevision;
	int mRebuilds;
};

} // namespace py_code
