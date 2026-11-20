// SPDX-License-Identifier: MIT
/* clipboard.h -- macOS general pasteboard, plain UTF-8 text. Main thread. */

#pragma once

#include <string>

namespace pyc {

std::string clipboard_text(); /* "" when there is no text */
void set_clipboard_text(const std::string &s);

} // namespace pyc
