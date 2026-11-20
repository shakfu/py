// SPDX-License-Identifier: GPL-3.0-only
/* py_complete.cpp -- see py_complete.h. */

#include "py_complete.h"

#include <algorithm>
#include <map>

namespace py_code {

/* Python keywords and builtins: they hold with no document words at all. */
static const char *const kLanguageNames[] = {
	"False", "None", "True", "and", "as", "assert", "async", "await", "break", "class",
	"continue", "def", "del", "elif", "else", "except", "finally", "for", "from", "global",
	"if", "import", "lambda", "nonlocal", "not", "pass", "raise", "return", "self", "try",
	"while", "with", "yield", "abs", "all", "any", "bool", "bytearray", "bytes", "callable",
	"chr", "classmethod", "dict", "dir", "divmod", "enumerate", "filter", "float", "format",
	"frozenset", "getattr", "globals", "hasattr", "hash", "isinstance", "issubclass", "iter",
	"len", "list", "locals", "map", "max", "min", "next", "object", "open", "print", "property",
	"range", "repr", "reversed", "round", "set", "setattr", "slice", "sorted", "staticmethod",
	"str", "sum", "super", "tuple", "type", "vars", "zip", "Exception", "ValueError",
	"TypeError", "KeyError", "IndexError", "RuntimeError", "NotImplementedError",
};

bool Completer::is_word_byte(unsigned char c) {
	return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

Completer::Completer() : mRevision(-1), mRebuilds(0) {}

void Completer::sync(const std::string &text, int revision) {
	if (revision == mRevision)
		return;
	mRevision = revision;
	++mRebuilds;

	/* Ordered map by name: the pool has to end up sorted for the lower_bound of
	 * `suggest`, and this way there is nothing to sort afterwards. */
	std::map<std::string, int> names;

	for (size_t i = 0; i < sizeof(kLanguageNames) / sizeof(kLanguageNames[0]); ++i)
		names[kLanguageNames[i]]; /* frequency 0: any word of the document wins */

	const int n = (int)text.size();
	int i = 0;
	while (i < n) {
		const unsigned char c = (unsigned char)text[(size_t)i];
		if (is_word_byte(c)) {
			int j = i;
			while (j < n && is_word_byte((unsigned char)text[(size_t)j]))
				j++;
			/* Tokens starting with a digit are numbers, not names. */
			if (!(c >= '0' && c <= '9'))
				names[text.substr((size_t)i, (size_t)(j - i))] += 1;
			i = j;
			continue;
		}
		i += 1;
	}

	mNames.clear();
	mNames.reserve(names.size());
	for (std::map<std::string, int>::const_iterator it = names.begin(); it != names.end(); ++it)
		mNames.push_back(Entry{it->first, it->second});
}

static bool starts_with(const std::string &s, const std::string &prefix) {
	return s.size() >= prefix.size() && s.compare(0, prefix.size(), prefix) == 0;
}

bool Completer::suggest(const std::string &text, int caret, int cycle, int min_prefix, Completion &out) const {
	out = Completion();

	const int len = (int)text.size();
	if (caret < 0)
		caret = 0;
	if (caret > len)
		caret = len;

	/* Token under the caret: the run of [A-Za-z0-9_] that ends there. */
	int start = caret;
	while (start > 0 && is_word_byte((unsigned char)text[(size_t)start - 1]))
		start--;

	const std::string prefix = text.substr((size_t)start, (size_t)(caret - start));
	if (min_prefix > 0 && (int)prefix.size() < min_prefix)
		return false;

	const std::vector<Entry> &pool = mNames;
	if (pool.empty())
		return false;

	/* Range [lower_bound(prefix), ...) that starts with the prefix. */
	const std::string probe = prefix;
	size_t lo = 0, hi = pool.size();
	while (lo < hi) {
		const size_t mid = (lo + hi) / 2;
		if (pool[mid].word < probe)
			lo = mid + 1;
		else
			hi = mid;
	}

	std::vector<const Entry *> matches;
	const size_t kMaxMatches = 256; /* cap: the ranking does not depend on the whole pool */
	for (size_t k = lo; k < pool.size(); ++k) {
		if (!starts_with(pool[k].word, prefix))
			break;
		if (pool[k].word.size() == prefix.size())
			continue; /* already written in full: not a suggestion */
		matches.push_back(&pool[k]);
		if (matches.size() >= kMaxMatches)
			break;
	}
	if (matches.empty())
		return false;

	/* Deterministic: frequency desc -> shorter -> alphabetical. */
	std::sort(matches.begin(), matches.end(), [](const Entry *a, const Entry *b) {
		if (a->freq != b->freq)
			return a->freq > b->freq;
		if (a->word.size() != b->word.size())
			return a->word.size() < b->word.size();
		return a->word < b->word;
	});

	const int count = (int)matches.size();
	int idx = cycle % count;
	if (idx < 0)
		idx += count;

	out.valid = true;
	out.start = start;
	out.end = caret;
	out.word = matches[(size_t)idx]->word;
	out.index = idx;
	out.alternatives = count;
	return true;
}

} // namespace py_code
