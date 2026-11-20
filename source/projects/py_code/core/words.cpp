// SPDX-License-Identifier: MIT
/* words.cpp -- see words.h. */

#include "core/words.h"

#include <algorithm>

namespace pyc {

const std::vector<std::string> &python_keywords() {
	static const std::vector<std::string> v = {
		"False", "None",   "True",    "and",      "as",       "assert", "async",  "await",
		"break", "case",   "class",   "continue", "def",      "del",    "elif",   "else",
		"except", "finally", "for",   "from",     "global",   "if",     "import", "in",
		"is",    "lambda", "match",   "nonlocal", "not",      "or",     "pass",   "raise",
		"return", "try",   "while",   "with",     "yield"};
	return v;
}

const std::vector<std::string> &python_builtins() {
	static const std::vector<std::string> v = {
		"abs", "aiter", "all", "anext", "any", "ascii", "bin", "bool", "breakpoint", "bytearray",
		"bytes", "callable", "chr", "classmethod", "compile", "complex", "delattr", "dict", "dir",
		"divmod", "enumerate", "eval", "exec", "filter", "float", "format", "frozenset", "getattr",
		"globals", "hasattr", "hash", "help", "hex", "id", "input", "int", "isinstance",
		"issubclass", "iter", "len", "list", "locals", "map", "max", "memoryview", "min", "next",
		"object", "oct", "open", "ord", "pow", "print", "property", "range", "repr", "reversed",
		"round", "set", "setattr", "slice", "sorted", "staticmethod", "str", "sum", "super",
		"tuple", "type", "vars", "zip", "__import__",
		"Ellipsis", "NotImplemented",
		"ArithmeticError", "AssertionError", "AttributeError", "BaseException", "BlockingIOError",
		"BrokenPipeError", "BufferError", "ChildProcessError", "ConnectionError", "DeprecationWarning",
		"EOFError", "Exception", "FileExistsError", "FileNotFoundError", "FloatingPointError",
		"GeneratorExit", "ImportError", "IndentationError", "IndexError", "InterruptedError",
		"IsADirectoryError", "KeyError", "KeyboardInterrupt", "LookupError", "MemoryError",
		"ModuleNotFoundError", "NameError", "NotADirectoryError", "NotImplementedError", "OSError",
		"OverflowError", "PermissionError", "RecursionError", "ReferenceError", "RuntimeError",
		"RuntimeWarning", "StopAsyncIteration", "StopIteration", "SyntaxError", "SystemError",
		"SystemExit", "TabError", "TimeoutError", "TypeError", "UnboundLocalError",
		"UnicodeDecodeError", "UnicodeEncodeError", "UnicodeError", "UserWarning", "ValueError",
		"Warning", "ZeroDivisionError"};
	return v;
}

static bool is_head(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_'; }
static bool is_tail(char c) { return is_head(c) || (c >= '0' && c <= '9'); }

void Words::sync(const std::string &text, int revision) {
	if (mSynced && revision == mRevision)
		return;
	mSynced = true;
	mRevision = revision;
	++mScans;
	mNames.clear();
	for (const std::string &b : python_builtins())
		mNames[b].kind = KindFunction;
	for (const std::string &k : python_keywords())
		mNames[k].kind = KindKeyword;
	const size_t n = text.size();
	for (size_t i = 0; i < n;) {
		/* An identifier cannot start right after a digit or another identifier byte. */
		if (is_head(text[i]) && (i == 0 || !is_tail(text[i - 1]))) {
			size_t j = i + 1;
			while (j < n && is_tail(text[j]))
				++j;
			++mNames[text.substr(i, j - i)].freq;
			i = j;
		} else {
			++i;
		}
	}
}

int Words::count(const std::string &name) const {
	auto it = mNames.find(name);
	return it == mNames.end() ? 0 : it->second.freq;
}

std::vector<CompletionItem> Words::query(const std::string &prefix, int limit) const {
	std::vector<std::pair<const std::string *, const Entry *>> hits;
	for (const auto &kv : mNames)
		if (kv.first != prefix && kv.first.compare(0, prefix.size(), prefix) == 0)
			hits.emplace_back(&kv.first, &kv.second);
	std::sort(hits.begin(), hits.end(), [](const auto &a, const auto &b) {
		if (a.second->freq != b.second->freq)
			return a.second->freq > b.second->freq;
		if (a.first->size() != b.first->size())
			return a.first->size() < b.first->size();
		return *a.first < *b.first;
	});
	if (limit > 0 && (int)hits.size() > limit)
		hits.resize((size_t)limit);
	std::vector<CompletionItem> out;
	for (const auto &h : hits) {
		CompletionItem it;
		it.label = it.insert = *h.first;
		it.kind = h.second->kind;
		out.push_back(it);
	}
	return out;
}

} // namespace pyc
