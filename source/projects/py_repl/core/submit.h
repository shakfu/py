// SPDX-License-Identifier: MIT
/* submit.h -- what Return does in the input. Spec: py_repl-spec.md, 2.3. */

#pragma once

namespace pyc {

class Editor;

/* repl_complete's answer (PY_REPL_COMPLETE/_INCOMPLETE/_INVALID), or Unknown
 * when no py object is linked. */
enum class Completeness { Complete, Incomplete, Invalid, Unknown };

enum class ReturnAction { Submit, Newline };

/* `force` is Cmd+Return. A newline when the caret is before the last
 * non-whitespace character or the input is incomplete; else submit. */
ReturnAction return_action(const Editor &input, Completeness c, bool force);

} // namespace pyc
