// SPDX-License-Identifier: MIT
/* py_code.h -- shared state of the py_code Max object (see docs/dev/py_code-spec.md 3). */

#pragma once

#include "ext.h"
#include "ext_obex.h"
#include "jgraphics.h"
#include "jpatcher_api.h"

#include "core/completion.h"
#include "core/editor.h"
#include "core/layout.h"
#include "core/palette.h"
#include "core/snippets.h"
#include "core/words.h"
#include "lsp/client.h"
#include "py_syntax.h"

#include <memory>
#include <string>

namespace pyc {

/* Font metrics, measured once per font size. */
struct Metrics {
	double size = -1;
	double cw = 7, lh = 15, base = 12; /* cell width, line height, baseline from row top */
	t_jfont *font = nullptr;
};

/* Box-local geometry derived from the box size, metrics and attributes. */
struct Geom {
	double w = 0, h = 0;
	double title_h = 0, status_h = 0;
	double gutter_w = 0;
	double text_x = 0, text_r = 0; /* text area left edge (row cell 0) and right edge */
	double view_y = 0, view_h = 0; /* rows area */
	double sb_x = 0;               /* scroll bar left edge */
	int cols = 0;                  /* wrap width in cells, 0 = no wrap */
};

/* C++ state of one box; heap allocated, owned by the Max struct. */
struct State {
	long id = 0; /* key in the box registry */

	Editor ed; /* declared before `layout`: Layout points to it */
	Layout layout;
	py_code::Syntax syntax;
	Words words;
	CompletionSession comp;
	Metrics m;
	double box_w = 420, box_h = 140; /* last known size (clocks have no view) */
	double hscroll = 0;              /* pixels, only when wrap is off */
	int goal = -1;                   /* goal cell for visual Up/Down */

	bool ready = false;     /* new() finished */
	bool focused = false;
	bool no_reload = false; /* set @file without reading it */

	/* Snippets. */
	Snippets snips;
	std::string snip_path; /* resolved path of the loaded file */
	long long snip_mtime = 0, snip_size = -1;

	/* LSP. */
	std::shared_ptr<lsp::Document> doc;
	lsp::State shown_state = lsp::State::Stopped;
	int shown_serial = -1;
	bool error_posted = false;
	std::string tmp_path;
	int req_rev = -1, req_caret = -1; /* pending completion request */
	lsp::Signature sig;

	/* Clocks (owned, freed in free). */
	t_clock *sync_clock = nullptr;   /* set_text 300 ms after the last edit */
	t_clock *write_clock = nullptr;  /* autowrite 400 ms after the last edit */
	t_clock *sig_clock = nullptr;    /* signature help 150 ms after edit/move */
	t_clock *blink_clock = nullptr;  /* evaluation flash, every 25 ms */
	t_clock *scroll_clock = nullptr; /* autoscroll while drag-selecting */
	bool write_pending = false;

	/* Evaluation flash. */
	bool blinking = false;
	int blink_start = 0, blink_end = 0;
	unsigned blink_t0 = 0;

	/* Mouse. */
	bool selecting = false, thumb_drag = false;
	double thumb_grab = 0;
	double mouse_x = 0, mouse_y = 0;
	int clicks = 0;
	unsigned click_t = 0;
	double click_x = 0, click_y = 0;
	bool autoscroll = false; /* scroll_clock armed */
};

} // namespace pyc

struct t_py_code {
	t_jbox box;
	void *outlet;
	pyc::State *st;

	/* Attributes. */
	t_symbol *theme;
	double fontsize;
	long tabwidth;
	long linenumbers, titlebar, matchbrackets, wrap, blink, completion, statusbar, autoclose, autoload;
	long readonly, autowrite;
	long blinkduration;
	double scrollspeed;
	t_symbol *file;
	t_symbol *snippets;
	t_symbol *lsp;
	t_jrgba colors[pyc::RoleCount];
};

namespace pyc {

constexpr double kPadY = 4;     /* Layout pad above the first and below the last row */
constexpr double kPadX = 6;     /* text inset from the gutter */
constexpr double kScrollW = 7;  /* scroll bar width */
constexpr int kPopupRows = 8;

/* py_code.cpp */
t_py_code *find_box(long id); /* null once freed */
void ensure_metrics(t_py_code *x);
Geom geometry(t_py_code *x, double w, double h);
Geom sync_layout(t_py_code *x); /* layout updated to the last known size */
void view_size(t_py_code *x, t_object *patcherview);
t_jrgba color(t_py_code *x, int role, double alpha_mul = 1);
void redraw(t_py_code *x);
void after_edit(t_py_code *x);  /* text changed: clocks, dirty flag */
void after_move(t_py_code *x);  /* caret moved: signature clock */
void reveal_caret(t_py_code *x);
void evaluate(t_py_code *x, py_code::EvalMode mode);
bool complete_open(t_py_code *x, bool lsp_only); /* true: popup open or request pending */
void complete_accept(t_py_code *x);
void paste(t_py_code *x);
void copy(t_py_code *x, bool cut);
void menu_save(t_py_code *x);
void menu_load(t_py_code *x);
std::string home_dir();
int cells_between(const std::string &s, int a, int b);

/* py_code_paint.cpp */
void paint(t_py_code *x, t_object *patcherview);
void getdrawparams(t_py_code *x, t_object *patcherview, t_jboxdrawparams *params);

/* py_code_input.cpp */
long key(t_py_code *x, t_object *patcherview, long keycode, long modifiers, long textcharacter);
void mousedown(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mousedrag(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseup(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseenter(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mousemove(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mouseleave(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers);
void mousewheel(t_py_code *x, t_object *patcherview, t_pt pt, long modifiers, double dx, double dy);
void scroll_tick(t_py_code *x);
long acceptsdrag(t_py_code *x, t_object *drag, t_object *view);
void dropfile(t_py_code *x, t_object *drag, void *arg);

} // namespace pyc
