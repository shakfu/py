// SPDX-License-Identifier: MIT
/* filedialog.h -- modal macOS file panels. Main thread. Return a POSIX path or "". */

#pragma once

#include <string>

namespace pyc {

std::string save_dialog(const std::string &default_name); /* restricted to .py */
std::string open_dialog();                                 /* any file */

} // namespace pyc
