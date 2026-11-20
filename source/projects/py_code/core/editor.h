// SPDX-License-Identifier: MIT
/* editor.h -- document text, selection and undo history for py_code.
 *
 * Offsets are UTF-8 byte offsets, clamped and snapped to code point
 * boundaries. "Cells" are code points. Lines are separated by '\n'.
 * Undo is an edit log grouped by kind (see docs/dev/py_code-spec.md 1.1).
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace pyc {

enum class Move { Left, Right, WordLeft, WordRight, Up, Down, LineStart, LineEnd, DocStart, DocEnd };

/* UTF-8 helpers. A stray or truncated byte counts as one code point. */
int utf8_next(const std::string &s, int off); /* offset of the next code point */
int utf8_prev(const std::string &s, int off); /* offset of the previous code point */
int utf8_snap(const std::string &s, int off); /* clamp, then back to a boundary */
std::string utf8_encode(uint32_t cp);

class Editor {
  public:
	explicit Editor(const std::string &text = "");

	/* Text and lines. */
	const std::string &text() const { return mText; }
	int length() const { return (int)mText.size(); }
	int revision() const { return mRevision; }
	int line_count() const { return (int)mLineStarts.size(); }
	int line_start(int line) const;
	int line_end(int line) const; /* offset of the '\n' or of the end of text */
	int line_of(int offset) const;
	std::string line_text(int line) const;
	int cells_before(int offset) const;          /* code points from line start */
	int offset_at_cell(int line, int cell) const; /* clamped to the line */

	/* Selection: [sel_start, sel_end) between anchor and caret. */
	int anchor() const { return mAnchor; }
	int caret() const { return mCaret; }
	int sel_start() const { return mAnchor < mCaret ? mAnchor : mCaret; }
	int sel_end() const { return mAnchor < mCaret ? mCaret : mAnchor; }
	bool has_selection() const { return mAnchor != mCaret; }
	std::string selected_text() const;
	void set_caret(int offset, bool extend = false);
	void select(int anchor, int caret);
	void select_all();
	void select_word_at(int offset);
	void select_line_at(int offset);
	void move(Move m, bool extend = false);

	/* Edits. Each returns true if the text changed. */
	bool insert(const std::string &s);
	bool type_char(uint32_t codepoint, bool autoclose);
	bool backspace(bool autoclose, bool word, int tabwidth);
	bool delete_forward(bool word);
	bool newline(int indent); /* '\n' + `indent` spaces */
	bool indent(int tabwidth);
	bool outdent(int tabwidth);
	bool set_line_indent(int line, int spaces);
	void reset(const std::string &text);

	bool undo();
	bool redo();
	bool can_undo() const { return !mUndo.empty(); }
	bool can_redo() const { return !mRedo.empty(); }

	static constexpr int kHistoryLimit = 1000; /* records */

  private:
	enum class Group { None, Typing, Backspace, Delete };
	struct Record {
		int start;
		std::string removed, inserted;
		int anchor_before, caret_before, anchor_after, caret_after;
	};
	using Records = std::vector<Record>;

	void apply(int start, int end, const std::string &s);
	void edit(int start, int end, const std::string &s, int anchor, int caret, Group kind, bool join);
	void end_group() { mGroup = Group::None; }
	void collapse(int offset);
	void index_lines();
	int word_left(int offset) const;
	int word_right(int offset) const;
	int first_nonblank(int line) const;
	bool shift_lines(int tabwidth, bool add);
	int clamp(int offset) const { return utf8_snap(mText, offset); }

	std::string mText;
	std::vector<int> mLineStarts;
	int mRevision = 0;
	int mAnchor = 0, mCaret = 0;
	int mGoal = -1; /* goal cell for consecutive Up/Down */

	std::vector<Records> mUndo, mRedo;
	int mRecords = 0;           /* records in mUndo */
	Group mGroup = Group::None; /* open group kind, None = closed */
	bool mCanJoin = false;      /* last group may take a set_line_indent */
};

} // namespace pyc
