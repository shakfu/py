// SPDX-License-Identifier: MIT
/* types.h -- value types shared by the LSP client modules. */

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace pyc::lsp {

enum class State { Starting, Ready, Failed, Stopped };

struct Diagnostic {
	int start = 0; /* byte offsets into the text last sent */
	int end = 0;
	int severity = 1; /* 1 error, 2 warning, 3 info, 4 hint */
	std::string message;
};

struct Signature {
	std::string label; /* empty = no signature */
	std::vector<std::pair<int, int>> params; /* byte ranges in label; {-1, -1} = not found */
	int active = -1; /* index into params, -1 = none */
};

} // namespace pyc::lsp
