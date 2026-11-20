// SPDX-License-Identifier: MIT
/* words.h -- local completion source: document identifiers, keywords, builtins. */

#pragma once

#include "core/completion_item.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace pyc {

const std::vector<std::string> &python_keywords();
const std::vector<std::string> &python_builtins();

class Words {
  public:
	enum Kind { KindText = 1, KindFunction = 3, KindKeyword = 14 }; /* LSP CompletionItemKind */

	/* Rescans `text` only when `revision` differs from the last sync. */
	void sync(const std::string &text, int revision);

	/* Names starting with `prefix` (case-sensitive), `prefix` itself excluded,
	 * by frequency desc, length asc, name. `limit <= 0`: no limit. */
	std::vector<CompletionItem> query(const std::string &prefix, int limit) const;

	int count(const std::string &name) const; /* occurrences in the document */
	int scans() const { return mScans; }

  private:
	struct Entry {
		int freq = 0;
		int kind = KindText;
	};
	std::unordered_map<std::string, Entry> mNames;
	int mRevision = -1;
	bool mSynced = false;
	int mScans = 0;
};

} // namespace pyc
