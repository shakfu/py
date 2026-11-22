// SPDX-License-Identifier: MIT
/* transcript.h -- the console's past input and output, as text plus kind spans.
 *
 * Spec: docs/dev/py_repl-spec.md, 2.1. Byte offsets. Every byte of text()
 * belongs to exactly one span; spans are ordered and adjacent.
 */

#pragma once

#include <string>
#include <vector>

namespace pyc {

enum class Kind { Input, Out, Err, Result, Traceback, Info };

struct Span {
	int start;
	int end;
	Kind kind;
	bool operator==(const Span &o) const { return start == o.start && end == o.end && kind == o.kind; }
};

class Transcript {
  public:
	explicit Transcript(int max_lines = 10000) : mMaxLines(max_lines) {}

	/* On a new line, `code` and a newline, as one Input span. */
	void append_input(const std::string &code);
	/* Verbatim; joins the previous span of the same kind unless it is Input. */
	void append(Kind kind, const std::string &text);
	/* Ends the text with a newline, so the next input starts on its own line. */
	void end_run();
	/* end_run(), then `text` and a newline as Info. */
	void info(const std::string &text);
	void clear();

	void set_max_lines(int n);
	int max_lines() const { return mMaxLines; }

	const std::string &text() const { return mText; }
	const std::vector<Span> &spans() const { return mSpans; }
	int revision() const { return mRevision; }
	int newline_count() const { return mNewlines; }

  private:
	void push(Kind kind, const std::string &text, bool join);
	void ensure_newline();
	void trim();

	std::string mText;
	std::vector<Span> mSpans;
	int mRevision = 0;
	int mNewlines = 0;
	int mMaxLines;
};

const char *kind_name(Kind k); /* for tests and debugging */

} // namespace pyc
