// SPDX-License-Identifier: MIT
/* completion_item.h -- one completion candidate, shared by core/ and lsp/. */

#pragma once

#include <string>

namespace pyc {

struct CompletionItem {
	std::string label;      /* shown */
	std::string insert;     /* inserted text (plain, no snippets) */
	std::string detail;     /* shown dimmed after the label */
	std::string filter;     /* matched against the prefix ("" = label) */
	std::string sort;       /* server order key ("" = keep order) */
	int kind = 0;           /* LSP CompletionItemKind */
	int replace_start = -1; /* byte range to replace; -1 = the word prefix */
	int replace_end = -1;
};

} // namespace pyc
