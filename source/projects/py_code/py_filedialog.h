// SPDX-License-Identifier: MIT
/* py_filedialog.h -- save/open panels for `.py` files.
 *
 * NSSavePanel/NSOpenPanel directly, not the SDK `saveas_dialog`: Max maps
 * types through `max-fileformats.txt`, where `.py` is not a Max type, so its
 * panel neither proposes nor enforces the extension. Both calls are modal and
 * must run on the main thread. */

#pragma once

#include <string>

namespace py_code {

/* Save panel restricted to `.py`. Returns the chosen POSIX path, "" if cancelled. */
std::string dialog_save_code(const std::string &default_name);

/* Open panel, `.py` preferred, other types allowed. Returns the path or "". */
std::string dialog_open_code();

} // namespace py_code
