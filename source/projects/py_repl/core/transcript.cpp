// SPDX-License-Identifier: MIT
/* transcript.cpp -- see transcript.h. */

#include "core/transcript.h"

#include <algorithm>

namespace pyc {

const char *kind_name(Kind k) {
	switch (k) {
	case Kind::Input: return "input";
	case Kind::Out: return "out";
	case Kind::Err: return "err";
	case Kind::Result: return "result";
	case Kind::Traceback: return "traceback";
	case Kind::Info: return "info";
	}
	return "?";
}

void Transcript::push(Kind kind, const std::string &text, bool join) {
	if (text.empty())
		return;
	const int start = (int)mText.size();
	mText += text;
	mNewlines += (int)std::count(text.begin(), text.end(), '\n');
	if (join && !mSpans.empty() && mSpans.back().kind == kind && kind != Kind::Input)
		mSpans.back().end = (int)mText.size();
	else
		mSpans.push_back(Span{start, (int)mText.size(), kind});
	++mRevision;
	trim();
}

void Transcript::ensure_newline() {
	if (mText.empty() || mText.back() == '\n')
		return;
	mText += '\n';
	++mNewlines;
	mSpans.back().end = (int)mText.size(); /* the newline ends the last span */
	++mRevision;
}

void Transcript::append_input(const std::string &code) {
	ensure_newline();
	push(Kind::Input, code.empty() || code.back() != '\n' ? code + "\n" : code, false);
}

void Transcript::append(Kind kind, const std::string &text) {
	if (kind == Kind::Input)
		append_input(text);
	else
		push(kind, text, true);
}

void Transcript::end_run() { ensure_newline(); }

void Transcript::info(const std::string &text) {
	ensure_newline();
	push(Kind::Info, text.empty() || text.back() != '\n' ? text + "\n" : text, true);
}

void Transcript::clear() {
	mText.clear();
	mSpans.clear();
	mNewlines = 0;
	++mRevision;
}

void Transcript::set_max_lines(int n) {
	mMaxLines = std::max(1, n);
	trim();
}

/* Past the cap by a quarter: drop whole lines from the front down to the cap.
 * The slack keeps trimming, an O(size) erase, rare under steady output. */
void Transcript::trim() {
	if (mNewlines <= mMaxLines + mMaxLines / 4)
		return;
	int drop = mNewlines - mMaxLines;
	size_t cut = 0;
	for (; drop > 0; --drop)
		cut = mText.find('\n', cut) + 1;
	const int c = (int)cut;
	mText.erase(0, cut);
	mNewlines = mMaxLines;
	std::vector<Span> kept;
	for (const Span &s : mSpans) {
		if (s.end <= c)
			continue;
		kept.push_back(Span{std::max(0, s.start - c), s.end - c, s.kind});
	}
	mSpans.swap(kept);
	++mRevision;
}

} // namespace pyc
