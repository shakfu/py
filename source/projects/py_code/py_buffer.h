// SPDX-License-Identifier: GPL-3.0-only
/* py_buffer.h -- UTF-8 text buffer of `py_code`.
 *
 * Invariants:
 *   - Offsets are bytes and always fall on a code point boundary.
 *   - `line_count()` is count('\n') + 1; a line excludes its '\n'.
 *   - Selection = anchor + caret, in either order; text is [min, max).
 *   - Undo stores full snapshots (50 levels). A typing or deleting burst is one
 *     step; any caret movement ends the burst. */

#pragma once

#include <string>
#include <vector>

namespace py_code {

class Buffer {
  public:
	static const int kUndoLimit = 50;

	explicit Buffer(const std::string &text = std::string());

	/* --- documento --------------------------------------------------------- */

	const std::string &text() const { return mText; }
	int length() const { return (int)mText.size(); }

	/* Counter that goes up on EVERY text change (not on caret moves or selection
	 * changes). The caches that depend on the whole document (e.g. the bracket
	 * matching) use it to invalidate themselves without copying the text. */
	int revision() const { return mRevision; }

	/* Replaces everything and resets caret, selection and undo (a load). */
	void set_text(const std::string &text);

	/* Auto-close of pairs. Off by default; the object turns it on (`@autoclose`). */
	void set_auto_close(bool on) { mAutoClose = on; }
	bool auto_close() const { return mAutoClose; }

	int line_count() const;
	int line_start(int line) const;  /* offset of the first byte of the line */
	int line_length(int line) const; /* bytes of the line, WITHOUT the '\n' */
	int line_end(int line) const { return line_start(line) + line_length(line); }
	std::string line_text(int line) const;

	int line_of_offset(int offset) const;
	int col_of_offset(int offset) const;           /* bytes from the start of the line */
	int offset_of(int line, int col) const;        /* (line, column) -> offset, clamp */

	/* VISUAL column = code points from the start of the line. It is the one paint
	 * and the mouse use (the font is monospace: each code point is a cell);
	 * `col_of_offset` instead counts BYTES, which is what the parser consumes. Without this, an accent on the line shifts the caret to the right. */
	int display_col_of_offset(int offset) const;
	int offset_of_display_col(int line, int col) const; /* inverse, clamped */

	/* Ranges for double and triple click. `start`/`end` are byte offsets.
	 * `word_range_at`: the run of [A-Za-z0-9_] or, if the offset falls on
	 * punctuation/space, the run of non-word-not-'\n' (so a double click on a
	 * parenthesis selects something instead of nothing). A '\n' never expands:
	 * `start == end == offset`. If the offset is at the end of a word, it selects it
	 * anyway. `line_range_at`: the caret line WITHOUT the trailing
	 * '\n' (triple click). */
	void word_range_at(int offset, int &start, int &end) const;
	void line_range_at(int offset, int &start, int &end) const;

	/* --- caret and selection ------------------------------------------------- */

	int caret() const { return mCaret; }
	int anchor() const { return mAnchor; }
	bool has_selection() const { return mCaret != mAnchor; }
	int selection_start() const { return mCaret < mAnchor ? mCaret : mAnchor; }
	int selection_end() const { return mCaret < mAnchor ? mAnchor : mCaret; }
	std::string selected_text() const;

	/* Collapses the selection and places the caret (clamp + code point
	 * boundary). */
	void set_caret(int offset);
	/* `extend` = Shift-: moves the moving end and leaves the anchor still. */
	void move_caret(int offset, bool extend = false);
	void set_selection(int start, int end); /* anchor = start, caret = end */
	void select_all();

	/* --- editing (true = the text changed) ---------------------------------- */

	bool insert_text(const std::string &text); /* deletes the selection first */
	/* One code point, encoded to UTF-8. With auto-close: ( [ { insert the pair
	 * (not before a word); quotes pair unless they follow a word that is not a
	 * string prefix (`don't` vs `f"`); a third quote closes as `"""|"""`; a
	 * selection is wrapped; typing a closer or quote over the same character
	 * moves past it. */
	bool insert_char(unsigned int codepoint);
	bool backspace(); /* with auto-close: Backspace between an empty pair deletes both */
	bool delete_forward();
	bool delete_selection();
	bool insert_tab(int tabwidth); /* `tabwidth` spaces (decision: Tab = spaces) */
	/* Return: inserts "\n" + `indent`. If the text before the caret ends in `{ (
	 * [` and there is a closer right after (with only spaces in between), the closer
	 * moves to its own line with the indentation of the current line: `{}` with the
	 * caret inside and indent " " => `{\n \n}`. The caller computes `indent`
	 * (`Syntax::indent_at`), so the buffer has no parser dependency. */
	bool insert_newline(const std::string &indent);
	/* Replaces the leading whitespace of `line` with `indent` (after a closer or
	 * `else:` was typed). Coalesced with the typing burst: no undo step of its own. */
	bool reindent_line(int line, const std::string &indent);
	bool outdent(int tabwidth);    /* Shift-Tab: removes up to `tabwidth` spaces from the start */

	/* --- movimiento -------------------------------------------------------- */

	void move_left(bool extend = false);
	void move_right(bool extend = false);
	void move_word_left(bool extend = false);  /* Alt-<- */
	void move_word_right(bool extend = false); /* Alt--> */
	void move_up(bool extend = false);         /* conserva la columna deseada */
	void move_down(bool extend = false);
	void move_line_start(bool extend = false); /* Home / Cmd-<- */
	void move_line_end(bool extend = false);   /* End / Cmd--> */
	void move_document_start(bool extend = false);
	void move_document_end(bool extend = false);

	/* --- undo / redo ------------------------------------------------------- */

	bool can_undo() const { return !mUndo.empty(); }
	bool can_redo() const { return !mRedo.empty(); }
	int undo_depth() const { return (int)mUndo.size(); }
	bool undo();
	bool redo();

	/* --- UTF-8 (public: used by paint and by the Max object) ------------------ */

	/* Next / previous code point boundary. They always advance at least 1 byte
	 * (invalid UTF-8 text => treated as loose bytes, it never hangs). */
	static int next_codepoint(const std::string &s, int offset);
	static int prev_codepoint(const std::string &s, int offset);
	static std::string encode_utf8(unsigned int codepoint);
	static bool is_word_byte(unsigned char c); /* [A-Za-z0-9_] */

  private:
	struct State {
		std::string text;
		int caret;
		int anchor;
	};

	/* What is being edited, for undo coalescing per burst. */
	enum RunTag { RunNone = 0, RunInsert, RunBackspace, RunDelete };

	void ensure_lines() const;
	void invalidate_lines();
	void note_edit(RunTag tag); /* push del snapshot previo, salvo coalescing */
	void finish_edit();
	void push_state(const State &s);
	void restore(const State &s);
	int clamp_offset(int offset) const; /* clamp + frontera de code point */
	bool delete_selection_impl();       /* no undo step of its own (insert_text uses it) */
	void set_caret_internal(int offset, bool extend);
	void move_vertical(int delta, bool extend);
	void reset_goal() { mGoalCol = -1; }

	std::string mText;
	int mCaret;
	int mAnchor;
	int mRevision; /* ++ in invalidate_lines(): only when the text changed */

	mutable std::vector<int> mLineStarts;
	mutable bool mLinesDirty;

	int mGoalCol; /* desired column for Up/Down; -1 = recompute from the caret */

	std::vector<State> mUndo;
	std::vector<State> mRedo;
	RunTag mRun;
	bool mAutoClose;
};

} // namespace py_code
