// SPDX-License-Identifier: MIT
/* submit.cpp -- see submit.h. */

#include "core/submit.h"

#include "core/editor.h"

namespace pyc {

ReturnAction return_action(const Editor &input, Completeness c, bool force) {
	if (force)
		return ReturnAction::Submit;
	const std::string &t = input.text();
	const size_t last = t.find_last_not_of(" \t\r\n");
	if (last != std::string::npos && (size_t)input.caret() <= last)
		return ReturnAction::Newline;
	return c == Completeness::Incomplete ? ReturnAction::Newline : ReturnAction::Submit;
}

} // namespace pyc
