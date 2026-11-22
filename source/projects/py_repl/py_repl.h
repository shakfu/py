// SPDX-License-Identifier: MIT
/* py_repl.h -- shared state of the py_repl Max object (docs/dev/py_repl-spec.md 3). */

#pragma once

#include "ext.h"
#include "ext_obex.h"
#include "jgraphics.h"
#include "jpatcher_api.h"

#include "core/completion.h"
#include "core/editor.h"
#include "core/history.h"
#include "core/layout.h"
#include "core/palette.h"
#include "core/stack.h"
#include "core/submit.h"
#include "core/transcript.h"
#include "core/words.h"
#include "platform/keys.h"
#include "py_syntax.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace pyc {

/* Font metrics, measured once per font size (as py_code). */
struct Metrics {
	double size = -1;
	double cw = 7, lh = 15, base = 12; /* cell width, line height, baseline from row top */
	t_jfont *font = nullptr;
};

/* Box-local geometry. */
struct Geom {
	double w = 0, h = 0;
	double title_h = 0;
	double prompt_x = 0;           /* `>>> ` / `... ` */
	double text_x = 0, text_r = 0; /* row cell 0 and the right edge */
	double view_y = 0, view_h = 0; /* rows area */
	double sb_x = 0;               /* scroll bar left edge */
	int cols = 0;                  /* wrap width in cells, 0 = no wrap */
};

enum Part { PartTranscript = 0, PartInput = 1 };

/* C++ state of one box; owned by the Max struct. */
struct State {
	long id = 0;

	/* Transcript: the model, and an Editor mirror for layout and selection. */
	Transcript tr;
	Editor tr_ed; /* before tr_layout: Layout points to it */
	Layout tr_layout;
	int tr_rev = -1; /* tr revision mirrored into tr_ed */

	Editor in_ed; /* before in_layout */
	Layout in_layout;
	py_code::Syntax in_syntax;

	/* Input spans of the transcript, tokenized once, keyed by their text. */
	py_code::Syntax scratch;
	int scratch_rev = 0;
	std::unordered_map<std::string, std::vector<std::vector<py_code::Token>>> tokens;

	Stack stack;
	History hist;
	Words words;
	int words_rev = 0;
	CompletionSession comp;
	Metrics m;
	double box_w = 420, box_h = 220;
	int goal = -1;
	int focus = PartInput;

	bool ready = false;
	bool focused = false;
	bool running = false;
	unsigned run_end = 0; /* systime_ms: a queued Ctrl+C after a run is dropped */

	/* Link: the repl_abi check is repeated when @py changes. */
	t_symbol *abi_for = nullptr;
	bool abi_ok = false;

	InterruptMonitor monitor;

	/* Mouse. */
	bool selecting = false, thumb_drag = false;
	int sel_part = PartInput;
	double thumb_grab = 0;
	double mouse_x = 0, mouse_y = 0;
	int clicks = 0;
	unsigned click_t = 0;
	double click_x = 0, click_y = 0;
	bool autoscroll = false;
	t_clock *scroll_clock = nullptr;
};

} // namespace pyc

struct t_py_repl {
	t_jbox box;
	pyc::State *st;

	/* Attributes. */
	t_symbol *py;
	t_symbol *theme;
	double fontsize;
	long tabwidth;
	long autoclose, completion, wrap, titlebar, savehistory;
	long maxlines;
	double scrollspeed;
	t_jrgba colors[pyc::RoleCount];
	t_jrgba promptcolor; /* not a palette role: follows the theme's comment color */
};

namespace pyc {

constexpr double kPadY = 4;    /* scroll pad above the first and below the last row */
constexpr double kPadX = 6;    /* inset of the prompt from the box edge */
constexpr double kScrollW = 7; /* scroll bar width */
constexpr int kPromptCells = 4;
constexpr int kPopupRows = 8;
constexpr int kSavedHistory = 200;

/* py_repl.cpp */
t_py_repl *find_box(long id); /* null once freed */
void ensure_metrics(t_py_repl *x);
Geom geometry(t_py_repl *x, double w, double h);
Geom sync_layout(t_py_repl *x); /* mirror, layouts and stack brought up to date */
void view_size(t_py_repl *x, t_object *patcherview);
t_jrgba color(t_py_repl *x, int role, double alpha_mul = 1);
void redraw(t_py_repl *x);
void reveal_caret(t_py_repl *x);
Editor &part_editor(t_py_repl *x, int part);
Layout &part_layout(t_py_repl *x, int part);

enum class Link { Ok, NotFound, AbiMismatch };
t_object *target(t_py_repl *x, Link *link); /* the linked py object, or null */
Completeness completeness(t_py_repl *x, const std::string &code);
void submit_input(t_py_repl *x); /* Return: the input becomes a run */
void submit_code(t_py_repl *x, const std::string &code);
void interrupt_input(t_py_repl *x); /* Ctrl+C while idle */
bool complete_open(t_py_repl *x);
void complete_accept(t_py_repl *x);
void history_step(t_py_repl *x, bool up); /* replaces the input */
void clear_transcript(t_py_repl *x);
void paste(t_py_repl *x);
void copy(t_py_repl *x, bool cut);
int cells_between(const std::string &s, int a, int b);
const std::vector<std::vector<py_code::Token>> &span_tokens(t_py_repl *x, const std::string &code);

/* py_repl_paint.cpp */
void paint(t_py_repl *x, t_object *patcherview);
void getdrawparams(t_py_repl *x, t_object *patcherview, t_jboxdrawparams *params);

/* py_repl_input.cpp */
long key(t_py_repl *x, t_object *patcherview, long keycode, long modifiers, long textcharacter);
void mousedown(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mousedrag(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseup(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseenter(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mousemove(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseleave(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers);
void mousewheel(t_py_repl *x, t_object *patcherview, t_pt pt, long modifiers, double dx, double dy);
void scroll_tick(t_py_repl *x);

} // namespace pyc
