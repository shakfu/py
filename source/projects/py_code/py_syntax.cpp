// SPDX-License-Identifier: MIT
/* py_syntax.cpp -- see py_syntax.h. */

#include "py_syntax.h"

#include <tree_sitter/api.h>

#include <algorithm>
#include <cstring>
#include <regex>

extern "C" const TSLanguage *tree_sitter_python(void);

namespace py_code {

/* Overlap rule: the smaller node wins; on equal size the later pattern wins.
 * So generic patterns go first and `(string)` loses to `(escape_sequence)`. */
static const char kHighlightQuery[] = R"scm(
((identifier) @class (#match? @class "^[A-Z]"))
((identifier) @number (#match? @number "^[A-Z][A-Z0-9_]*$"))
((identifier) @self (#any-of? @self "self" "cls"))

(decorator) @decorator

(call function: (identifier) @function)
(call function: (attribute attribute: (identifier) @function))
((call function: (identifier) @builtin)
 (#any-of? @builtin
  "abs" "aiter" "all" "anext" "any" "ascii" "bin" "bool" "breakpoint" "bytearray"
  "bytes" "callable" "chr" "classmethod" "compile" "complex" "delattr" "dict" "dir"
  "divmod" "enumerate" "eval" "exec" "filter" "float" "format" "frozenset" "getattr"
  "globals" "hasattr" "hash" "help" "hex" "id" "input" "int" "isinstance"
  "issubclass" "iter" "len" "list" "locals" "map" "max" "memoryview" "min" "next"
  "object" "oct" "open" "ord" "pow" "print" "property" "range" "repr" "reversed"
  "round" "set" "setattr" "slice" "sorted" "staticmethod" "str" "sum" "super"
  "tuple" "type" "vars" "zip" "__import__"))

(function_definition name: (identifier) @function)
(class_definition name: (identifier) @class)
(type (identifier) @class)

[(none) (true) (false)] @builtin
[(integer) (float)] @number

(string) @string
(escape_sequence) @escape
(comment) @comment

["(" ")" "[" "]" "{" "}"] @bracket
(interpolation) @default
(interpolation "{" @escape "}" @escape)

["-" "-=" "!=" "*" "**" "**=" "*=" "/" "//" "//=" "/=" "&" "&=" "%" "%=" "^" "^="
 "+" "->" "+=" "<" "<<" "<<=" "<=" "<>" "=" ":=" "==" ">" ">=" ">>" ">>=" "|" "|="
 "~" "@=" "and" "in" "is" "not" "or" "is not" "not in"] @operator

["as" "assert" "async" "await" "break" "class" "continue" "def" "del" "elif" "else"
 "except" "finally" "for" "from" "global" "if" "import" "lambda" "nonlocal" "pass"
 "raise" "return" "try" "while" "with" "yield" "match" "case"] @keyword
)scm";

const char *token_type_name(TokenType t) {
	static const char *const names[TokenTypeCount] = {
		"default", "keyword", "builtin", "function", "class",   "decorator", "string",
		"escape",  "number",  "self",    "comment",  "bracket", "operator"};
	return (t >= 0 && t < TokenTypeCount) ? names[t] : "?";
}

/* TokenTypeCount = an unknown capture name, ignored. `@default` paints the
 * text color over a larger capture (f-string expressions inside `@string`). */
static TokenType type_for_capture(const std::string &name) {
	for (int t = 0; t < TokenTypeCount; ++t)
		if (name == token_type_name((TokenType)t))
			return (TokenType)t;
	return TokenTypeCount;
}

/* --- predicates (#match?, #eq?, #any-of?): the C library leaves them to us -- */

struct Syntax::Predicate {
	enum Op { Eq, Match, AnyOf } op;
	uint32_t capture;
	std::string value;
	std::regex re;
	std::vector<std::string> set;
};

void Syntax::build_query() {
	uint32_t err_offset = 0;
	TSQueryError err = TSQueryErrorNone;
	mQuery = ts_query_new(tree_sitter_python(), kHighlightQuery, (uint32_t)std::strlen(kHighlightQuery), &err_offset,
						  &err);
	if (!mQuery)
		return; /* a bug in kHighlightQuery: tests assert this never happens */

	const uint32_t ncap = ts_query_capture_count(mQuery);
	mCaptureType.resize(ncap);
	for (uint32_t i = 0; i < ncap; ++i) {
		uint32_t len = 0;
		const char *name = ts_query_capture_name_for_id(mQuery, i, &len);
		mCaptureType[i] = type_for_capture(std::string(name, len));
	}

	const uint32_t npat = ts_query_pattern_count(mQuery);
	mPredicates.resize(npat);
	for (uint32_t p = 0; p < npat; ++p) {
		uint32_t nsteps = 0;
		const TSQueryPredicateStep *steps = ts_query_predicates_for_pattern(mQuery, p, &nsteps);
		uint32_t i = 0;
		while (i < nsteps) {
			uint32_t j = i;
			while (j < nsteps && steps[j].type != TSQueryPredicateStepTypeDone)
				++j;
			/* steps[i] = operator, steps[i+1] = capture, then string arguments */
			if (j - i >= 3 && steps[i].type == TSQueryPredicateStepTypeString &&
				steps[i + 1].type == TSQueryPredicateStepTypeCapture) {
				uint32_t len = 0;
				const std::string op(ts_query_string_value_for_id(mQuery, steps[i].value_id, &len), len);
				Predicate pr;
				pr.capture = steps[i + 1].value_id;
				for (uint32_t k = i + 2; k < j; ++k) {
					const char *s = ts_query_string_value_for_id(mQuery, steps[k].value_id, &len);
					pr.set.emplace_back(s, len);
				}
				if (op == "eq?") {
					pr.op = Predicate::Eq;
					pr.value = pr.set[0];
				} else if (op == "match?") {
					pr.op = Predicate::Match;
					pr.re = std::regex(pr.set[0]);
				} else {
					pr.op = Predicate::AnyOf;
				}
				mPredicates[p].push_back(std::move(pr));
			}
			i = j + 1;
		}
	}
}

bool Syntax::predicates_hold(unsigned pattern, const void *m) const {
	const TSQueryMatch *match = (const TSQueryMatch *)m;
	for (const Predicate &pr : mPredicates[pattern]) {
		const TSQueryCapture *cap = nullptr;
		for (uint16_t k = 0; k < match->capture_count; ++k)
			if (match->captures[k].index == pr.capture)
				cap = &match->captures[k];
		if (!cap)
			return false;
		const uint32_t a = ts_node_start_byte(cap->node), b = ts_node_end_byte(cap->node);
		const std::string s = mText.substr(a, b - a);
		bool ok = false;
		switch (pr.op) {
		case Predicate::Eq:
			ok = s == pr.value;
			break;
		case Predicate::Match:
			ok = std::regex_search(s, pr.re);
			break;
		case Predicate::AnyOf:
			ok = std::find(pr.set.begin(), pr.set.end(), s) != pr.set.end();
			break;
		}
		if (!ok)
			return false;
	}
	return true;
}

/* --- lifecycle and parsing ----------------------------------------------- */

Syntax::Syntax() {
	mParser = ts_parser_new();
	ts_parser_set_language(mParser, tree_sitter_python());
	mCursor = ts_query_cursor_new();
	build_query();
	line_index();
}

Syntax::~Syntax() {
	if (mTree)
		ts_tree_delete(mTree);
	if (mQuery)
		ts_query_delete(mQuery);
	ts_query_cursor_delete(mCursor);
	ts_parser_delete(mParser);
}

void Syntax::line_index() {
	mLineStarts.assign(1, 0);
	for (size_t i = 0; i < mText.size(); ++i)
		if (mText[i] == '\n')
			mLineStarts.push_back((int)i + 1);
	mLineTokens.assign(mLineStarts.size(), std::vector<Token>());
	mLineValid.assign(mLineStarts.size(), false);
}

int Syntax::line_of(int offset) const {
	auto it = std::upper_bound(mLineStarts.begin(), mLineStarts.end(), offset);
	return (int)(it - mLineStarts.begin()) - 1;
}

static TSPoint point_of(const std::vector<int> &starts, int offset) {
	auto it = std::upper_bound(starts.begin(), starts.end(), offset);
	const int row = (int)(it - starts.begin()) - 1;
	return TSPoint{(uint32_t)row, (uint32_t)(offset - starts[(size_t)row])};
}

void Syntax::update(const std::string &text, int revision) {
	if (mTree && revision == mRevision)
		return;
	mRevision = revision;
	if (mTree && text == mText)
		return;

	if (mTree) {
		/* The edit is the span where old and new differ. */
		const size_t n0 = mText.size(), n1 = text.size(), lim = std::min(n0, n1);
		size_t p = 0;
		while (p < lim && mText[p] == text[p])
			++p;
		size_t s = 0;
		while (s < lim - p && mText[n0 - 1 - s] == text[n1 - 1 - s])
			++s;
		TSInputEdit e;
		e.start_byte = (uint32_t)p;
		e.old_end_byte = (uint32_t)(n0 - s);
		e.new_end_byte = (uint32_t)(n1 - s);
		e.start_point = point_of(mLineStarts, (int)p);
		e.old_end_point = point_of(mLineStarts, (int)(n0 - s));
		mText = text;
		line_index();
		e.new_end_point = point_of(mLineStarts, (int)(n1 - s));
		ts_tree_edit(mTree, &e);
	} else {
		mText = text;
		line_index();
	}
	TSTree *t = ts_parser_parse_string(mParser, mTree, mText.data(), (uint32_t)mText.size());
	if (mTree)
		ts_tree_delete(mTree);
	mTree = t;
	++mParses;
	std::fill(mLineValid.begin(), mLineValid.end(), false);
}

bool Syntax::has_error() const { return mTree && ts_node_has_error(ts_tree_root_node(mTree)); }

/* --- highlight ------------------------------------------------------------ */

const std::vector<Token> &Syntax::tokens(int line) {
	static const std::vector<Token> kEmpty;
	if (line < 0 || line >= line_count() || !mTree)
		return kEmpty;
	if (mLineValid[(size_t)line])
		return mLineTokens[(size_t)line];

	const int ls = mLineStarts[(size_t)line];
	int le = line + 1 < line_count() ? mLineStarts[(size_t)line + 1] - 1 : (int)mText.size();
	std::vector<Token> &out = mLineTokens[(size_t)line];
	out.clear();
	mLineValid[(size_t)line] = true;
	if (le <= ls)
		return out;

	struct Span {
		int a, b;
		uint32_t size, pattern;
		TokenType type;
	};
	std::vector<Span> spans;
	if (mQuery) {
		ts_query_cursor_set_byte_range(mCursor, (uint32_t)ls, (uint32_t)le);
		ts_query_cursor_exec(mCursor, mQuery, ts_tree_root_node(mTree));
		TSQueryMatch m;
		while (ts_query_cursor_next_match(mCursor, &m)) {
			if (!predicates_hold(m.pattern_index, &m))
				continue;
			for (uint16_t k = 0; k < m.capture_count; ++k) {
				const TokenType t = mCaptureType[m.captures[k].index];
				if (t == TokenTypeCount)
					continue;
				const int a = (int)ts_node_start_byte(m.captures[k].node);
				const int b = (int)ts_node_end_byte(m.captures[k].node);
				if (b <= ls || a >= le)
					continue;
				spans.push_back(Span{std::max(a, ls), std::min(b, le), (uint32_t)(b - a), m.pattern_index, t});
			}
		}
	}
	std::sort(spans.begin(), spans.end(), [](const Span &x, const Span &y) {
		return x.size != y.size ? x.size > y.size : x.pattern < y.pattern;
	});
	std::vector<unsigned char> role((size_t)(le - ls), (unsigned char)Default);
	for (const Span &s : spans)
		std::fill(role.begin() + (s.a - ls), role.begin() + (s.b - ls), (unsigned char)s.type);

	int i = 0;
	const int n = le - ls;
	while (i < n) {
		int j = i + 1;
		while (j < n && role[(size_t)j] == role[(size_t)i])
			++j;
		out.push_back(Token{i, j - i, (TokenType)role[(size_t)i]});
		i = j;
	}
	return out;
}

/* --- brackets ------------------------------------------------------------- */

static char partner_of(char c) {
	switch (c) {
	case '(': return ')';
	case '[': return ']';
	case '{': return '}';
	case ')': return '(';
	case ']': return '[';
	case '}': return '{';
	}
	return 0;
}

/* The bracket leaf exactly at `off`, or a null node. */
static TSNode bracket_leaf(TSNode root, int off, char *c) {
	TSNode n = ts_node_descendant_for_byte_range(root, (uint32_t)off, (uint32_t)off + 1);
	if (ts_node_is_null(n) || ts_node_child_count(n) != 0 || ts_node_is_named(n) || ts_node_is_missing(n) ||
		(int)ts_node_start_byte(n) != off || ts_node_end_byte(n) != (uint32_t)off + 1)
		return TSNode{};
	const char *t = ts_node_type(n);
	if (!t || std::strlen(t) != 1 || !partner_of(t[0]))
		return TSNode{};
	*c = t[0];
	return n;
}

BracketMatch Syntax::match_bracket(int caret) const {
	BracketMatch r;
	if (!mTree)
		return r;
	const TSNode root = ts_tree_root_node(mTree);
	char c = 0;
	TSNode n{};
	const int len = (int)mText.size();
	if (caret < len && (mText[(size_t)caret] == '(' || mText[(size_t)caret] == '[' || mText[(size_t)caret] == '{'))
		n = bracket_leaf(root, caret, &c);
	if (ts_node_is_null(n) && caret > 0 &&
		(mText[(size_t)caret - 1] == ')' || mText[(size_t)caret - 1] == ']' || mText[(size_t)caret - 1] == '}'))
		n = bracket_leaf(root, caret - 1, &c);
	if (ts_node_is_null(n))
		return r;

	/* Brackets pair up among siblings: nested pairs live inside child nodes, and
	 * an ERROR parent keeps flat tokens, so count depth over the siblings. */
	const TSNode parent = ts_node_parent(n);
	if (ts_node_is_null(parent))
		return r;
	const bool open = c == '(' || c == '[' || c == '{';
	const char want = partner_of(c);
	const uint32_t count = ts_node_child_count(parent);
	int self = -1;
	for (uint32_t i = 0; i < count; ++i)
		if (ts_node_eq(ts_node_child(parent, i), n))
			self = (int)i;
	if (self < 0)
		return r;
	int depth = 0;
	for (int i = self + (open ? 1 : -1); i >= 0 && i < (int)count; i += open ? 1 : -1) {
		const TSNode s = ts_node_child(parent, (uint32_t)i);
		if (ts_node_is_named(s) || ts_node_is_missing(s))
			continue;
		const char *t = ts_node_type(s);
		if (!t || std::strlen(t) != 1)
			continue;
		if (t[0] == c)
			++depth;
		else if (t[0] == want && depth-- == 0) {
			const int a = (int)ts_node_start_byte(n), b = (int)ts_node_start_byte(s);
			r.first = std::min(a, b);
			r.second = std::max(a, b);
			return r;
		}
	}
	return r;
}

/* --- indentation ---------------------------------------------------------- */

int leading_ws(const std::string &line) {
	int i = 0;
	while (i < (int)line.size() && (line[(size_t)i] == ' ' || line[(size_t)i] == '\t'))
		++i;
	return i;
}

int Syntax::line_indent(int line) const {
	const int a = mLineStarts[(size_t)line];
	int i = a;
	while (i < (int)mText.size() && (mText[(size_t)i] == ' ' || mText[(size_t)i] == '\t'))
		++i;
	return i - a;
}

static bool is_dedent_keyword(const char *t) {
	return !std::strcmp(t, "return") || !std::strcmp(t, "pass") || !std::strcmp(t, "break") ||
		   !std::strcmp(t, "continue") || !std::strcmp(t, "raise");
}

int Syntax::indent_at(int caret, int tab) const {
	caret = std::max(0, std::min(caret, (int)mText.size()));
	const int line = line_of(caret);
	const int ls = mLineStarts[(size_t)line];
	const int own = std::min(line_indent(line), caret - ls);
	/* Blank up to the caret, or a comment line: keep the indentation. */
	if (own == caret - ls || mText[(size_t)(ls + own)] == '#' || !mTree)
		return own;

	const TSNode root = ts_tree_root_node(mTree);
	for (TSNode n = ts_node_descendant_for_byte_range(root, (uint32_t)caret, (uint32_t)caret); !ts_node_is_null(n);
		 n = ts_node_parent(n))
		if (!std::strcmp(ts_node_type(n), "string") && (int)ts_node_start_byte(n) < caret &&
			caret < (int)ts_node_end_byte(n))
			return own;

	struct Open {
		char c;
		int row, col;
		int follow; /* column of the first leaf after it on its row, -1 = none */
	};
	std::vector<Open> stack;
	const char *last = nullptr; /* type of the last significant leaf */
	const char *first = nullptr; /* first leaf of the current logical line */
	int stmt_row = -1, prev_row = -1;
	bool continued = false;

	TSTreeCursor cur = ts_tree_cursor_new(root);
	bool done = false;
	while (!done) {
		const TSNode n = ts_tree_cursor_current_node(&cur);
		const int a = (int)ts_node_start_byte(n);
		if (a >= caret)
			break;
		if (ts_node_child_count(n) > 0 && ts_tree_cursor_goto_first_child(&cur))
			continue;
		/* a leaf */
		const int b = (int)ts_node_end_byte(n);
		const char *t = ts_node_type(n);
		if (b <= caret && b > a && std::strcmp(t, "comment") != 0) {
			const TSPoint p = ts_node_start_point(n);
			if (!std::strcmp(t, "line_continuation")) {
				continued = true;
			} else {
				if (stack.empty() && !continued && (prev_row < 0 || (int)p.row > prev_row)) {
					stmt_row = (int)p.row;
					first = t;
				}
				continued = false;
				if (!stack.empty() && stack.back().follow < 0 && stack.back().row == (int)p.row)
					stack.back().follow = (int)p.column;
				const char c = std::strlen(t) == 1 ? t[0] : 0;
				if (c == '(' || c == '[' || c == '{')
					stack.push_back(Open{c, (int)p.row, (int)p.column, -1});
				else if ((c == ')' || c == ']' || c == '}') && !stack.empty() && stack.back().c == partner_of(c))
					stack.pop_back();
				last = t;
				prev_row = (int)ts_node_end_point(n).row;
			}
		}
		while (!ts_tree_cursor_goto_next_sibling(&cur)) {
			if (!ts_tree_cursor_goto_parent(&cur)) {
				done = true;
				break;
			}
		}
	}
	ts_tree_cursor_delete(&cur);

	if (!stack.empty()) {
		const Open &o = stack.back();
		return o.follow >= 0 ? o.follow : line_indent(o.row) + tab;
	}
	if (stmt_row < 0)
		return own;
	const int base = line_indent(stmt_row);
	if (last && !std::strcmp(last, ":"))
		return base + tab;
	if (first && is_dedent_keyword(first))
		return std::max(0, base - tab);
	return base;
}

int Syntax::reindent_at(int caret, int tab) const {
	if (caret <= 0 || caret > (int)mText.size())
		return -1;
	const char c = mText[(size_t)caret - 1];
	const int line = line_of(caret - 1);
	const int ls = mLineStarts[(size_t)line];
	const int cur = line_indent(line);

	if (c == ')' || c == ']' || c == '}') {
		if (ls + cur != caret - 1)
			return -1; /* the closer is not the first thing on the line */
		const BracketMatch m = match_bracket(caret);
		if (!m.valid())
			return -1;
		const int want = line_indent(line_of(m.first));
		return want != cur ? want : -1;
	}

	if (c == ':' && cur > 0) {
		const int le = line + 1 < line_count() ? mLineStarts[(size_t)line + 1] - 1 : (int)mText.size();
		int k = ls + cur, e = k;
		while (e < le && ((mText[(size_t)e] >= 'a' && mText[(size_t)e] <= 'z')))
			++e;
		const std::string word = mText.substr((size_t)k, (size_t)(e - k));
		if (word != "else" && word != "elif" && word != "except" && word != "finally")
			return -1;
		/* Only when the line still sits at the indentation Return gave it: the
		 * previous non-blank line's. */
		for (int p = line - 1; p >= 0; --p) {
			const int pls = mLineStarts[(size_t)p];
			const int ple = mLineStarts[(size_t)p + 1] - 1;
			if (pls + line_indent(p) >= ple)
				continue;
			return line_indent(p) == cur ? std::max(0, cur - tab) : -1;
		}
	}
	return -1;
}

/* --- evaluation span ------------------------------------------------------ */

EvalSpan Syntax::eval_span(int caret, int sel_start, int sel_end, EvalMode mode) const {
	EvalSpan s;
	const int len = (int)mText.size();
	if (mode == EvalModeDocument) {
		if (len > 0) {
			s.start = 0;
			s.end = len;
		}
		return s;
	}
	if (sel_start >= 0 && sel_end > sel_start) {
		s.start = sel_start;
		s.end = std::min(sel_end, len);
		return s;
	}
	caret = std::max(0, std::min(caret, len));
	const int line = line_of(caret);

	if (mode == EvalModeBlock && mTree) {
		const TSNode root = ts_tree_root_node(mTree);
		const uint32_t count = ts_node_child_count(root);
		for (uint32_t i = 0; i < count; ++i) {
			const TSNode c = ts_node_child(root, i);
			if (!std::strcmp(ts_node_type(c), "comment"))
				continue;
			const int r0 = (int)ts_node_start_point(c).row, r1 = (int)ts_node_end_point(c).row;
			if (r0 <= line && line <= r1 && ts_node_end_byte(c) > ts_node_start_byte(c)) {
				s.start = mLineStarts[(size_t)r0];
				s.end = (int)ts_node_end_byte(c);
				return s;
			}
		}
	}

	const int a = mLineStarts[(size_t)line];
	const int b = line + 1 < line_count() ? mLineStarts[(size_t)line + 1] - 1 : len;
	if (a + line_indent(line) < b) {
		s.start = a;
		s.end = b;
	}
	return s;
}

static bool ends_with(const char *s, const char *suffix) {
	const size_t n = std::strlen(s), m = std::strlen(suffix);
	return n >= m && !std::strcmp(s + n - m, suffix);
}

/* This grammar puts a bare expression directly under `module` (no
 * expression_statement wrapper), so a statement is recognized by its type. */
bool Syntax::is_expression(const std::string &code) {
	TSTree *t = ts_parser_parse_string(mParser, nullptr, code.data(), (uint32_t)code.size());
	if (!t)
		return false;
	const TSNode root = ts_tree_root_node(t);
	bool ok = !ts_node_has_error(root);
	int statements = 0;
	const uint32_t count = ok ? ts_node_named_child_count(root) : 0;
	for (uint32_t i = 0; ok && i < count; ++i) {
		const char *type = ts_node_type(ts_node_named_child(root, i));
		if (!std::strcmp(type, "comment"))
			continue;
		++statements;
		if (ends_with(type, "_statement") || ends_with(type, "_definition") || !std::strcmp(type, "assignment") ||
			!std::strcmp(type, "augmented_assignment") || !std::strcmp(type, "yield"))
			ok = false;
	}
	ts_tree_delete(t);
	return ok && statements == 1;
}

/* --- dedent --------------------------------------------------------------- */

std::string dedent(const std::string &code) {
	int common = -1;
	size_t i = 0;
	while (i <= code.size()) {
		size_t e = code.find('\n', i);
		if (e == std::string::npos)
			e = code.size();
		const std::string line = code.substr(i, e - i);
		const int ws = leading_ws(line);
		if (ws < (int)line.size() && line[(size_t)ws] != '\r')
			common = common < 0 ? ws : std::min(common, ws);
		i = e + 1;
	}
	if (common <= 0)
		return code;
	std::string out;
	out.reserve(code.size());
	i = 0;
	while (i <= code.size()) {
		size_t e = code.find('\n', i);
		const bool last = e == std::string::npos;
		if (last)
			e = code.size();
		const std::string line = code.substr(i, e - i);
		out += line.substr(std::min((size_t)common, (size_t)leading_ws(line)));
		if (!last)
			out += '\n';
		i = e + 1;
	}
	return out;
}

} // namespace py_code
