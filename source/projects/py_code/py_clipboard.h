// SPDX-License-Identifier: MIT
/* py_clipboard.h -- system clipboard (implemented in py_clipboard.mm). */

#pragma once

#include <string>

namespace py_code {

/* UTF-8 plain text of the general clipboard; "" if there is no text or it is not
 * UTF-8. */
std::string clipboard_read_text();

/* Replaces the clipboard contents with `text` (UTF-8). false if it failed. */
bool clipboard_write_text(const std::string &text);

} // namespace py_code
