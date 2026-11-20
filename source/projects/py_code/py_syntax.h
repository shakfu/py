// SPDX-License-Identifier: MIT
/* py_syntax.h -- tree-sitter-python analysis for `py_code`.
 *
 * One `Syntax` holds a copy of the document and its parse tree. `update()`
 * reparses incrementally: the edit is recovered as the changed span between
 * the old and new text (common prefix and suffix), so `Buffer` needs no edit
 * log. Everything else answers from that tree:
 *
 *   - per-line highlight tokens (query below, cached per line per revision),
 *   - bracket pairs (only real bracket leaves: those in strings or comments
 *     are not leaves of type "(" etc., so they never match),
 *   - the indentation of a new line,
 *   - the span Cmd-Return evaluates (selection, top-level statement, line),
 *   - whether a code string is a single expression (`eval`) or not (`exec`).
 *
 * Offsets are UTF-8 bytes, the same unit as `Buffer`. No Max dependency.
 */

#pragma once

#include <string>
#include <vector>

struct TSParser;
struct TSTree;
struct TSQuery;
struct TSQueryCursor;

namespace py_code {

/* Highlight roles. Each maps to one color attribute of the object. */
enum TokenType {
	Default = 0,
	Keyword,
	Builtin,   /* print, len, None, True, ... */
	Function,  /* def name, call name */
	Class,     /* CapitalizedName, type annotation */
	Decorator,
	String,
	Escape,    /* \n, f-string braces */
	Number,    /* also ALL_CAPS constants */
	Self,      /* self, cls */
	Comment,
	Bracket,
	Operator,
	TokenTypeCount
};

const char *token_type_name(TokenType t);

/* Line-relative token. The tokens of a line cover it with no gaps. */
struct Token {
	int col;    /* byte offset inside the line */
	int length; /* bytes */
	TokenType type;
};

struct BracketMatch {
	int first = -1;
	int second = -1;
	bool valid() const { return first >= 0 && second >= 0; }
};

enum EvalMode {
	EvalModeBlock = 0,   /* Cmd-Return: selection, else top-level statement, else line */
	EvalModeLine = 1,    /* Shift-Return: selection, else line */
	EvalModeDocument = 2 /* Cmd-Shift-Return: everything */
};

struct EvalSpan {
	int start = -1; /* byte, inclusive */
	int end = -1;   /* byte, exclusive */
	bool has_value() const { return start >= 0 && end > start; }
};

class Syntax {
  public:
	Syntax();
	~Syntax();
	Syntax(const Syntax &) = delete;
	Syntax &operator=(const Syntax &) = delete;

	/* Brings the tree up to date. A no-op when `revision` is unchanged. */
	void update(const std::string &text, int revision);

	const std::string &text() const { return mText; }
	int line_count() const { return (int)mLineStarts.size(); }

	/* Tokens of `line`, cached until the next revision. */
	const std::vector<Token> &tokens(int line);

	/* Partner of the bracket after the caret (an opener) or before it (a closer). */
	BracketMatch match_bracket(int caret) const;

	/* Spaces for a new line inserted at `caret`. */
	int indent_at(int caret, int tabwidth) const;

	/* New indentation for the line of `caret` after a closer or a block keyword
	 * (`else:` etc.) was typed at `caret - 1`; -1 = leave it. */
	int reindent_at(int caret, int tabwidth) const;

	/* What to evaluate. `sel_start == sel_end` means no selection. */
	EvalSpan eval_span(int caret, int sel_start, int sel_end, EvalMode mode) const;

	/* True when `code` parses as one expression statement with no assignment, so
	 * `eval` (which returns a value) accepts it. Parses `code` on its own. */
	bool is_expression(const std::string &code);

	/* Test instrumentation. */
	int parses() const { return mParses; }
	bool has_error() const;

  private:
	struct Predicate;
	void build_query();
	void line_index();
	int line_of(int offset) const;
	int line_indent(int line) const;
	bool predicates_hold(unsigned pattern, const void *match) const;

	TSParser *mParser = nullptr;
	TSTree *mTree = nullptr;
	TSQuery *mQuery = nullptr;
	TSQueryCursor *mCursor = nullptr;
	std::vector<TokenType> mCaptureType;                 /* query capture index -> role */
	std::vector<std::vector<Predicate>> mPredicates;     /* per pattern */

	std::string mText;
	int mRevision = -1;
	int mParses = 0;
	std::vector<int> mLineStarts;
	std::vector<std::vector<Token>> mLineTokens;
	std::vector<bool> mLineValid;
};

/* Removes the leading whitespace common to every non-blank line (Python
 * `textwrap.dedent`), so an indented line or selection can be executed. */
std::string dedent(const std::string &code);

/* Spaces + tabs at the start of `line`. */
int leading_ws(const std::string &line);

} // namespace py_code
