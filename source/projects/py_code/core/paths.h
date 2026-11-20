// SPDX-License-Identifier: MIT
/* paths.h -- path helpers for Finder drops and attribute values. */

#pragma once

#include <string>
#include <vector>

namespace pyc {

/* `Macintosh HD:/Users/a/x.wav` -> `/Users/a/x.wav`; other input unchanged. */
std::string posix_path(const std::string &s);

/* `~` or `~/...` -> home + rest. Empty home: unchanged. `~user` unchanged. */
std::string expand_home(const std::string &home, const std::string &s);

/* Trims whitespace and drops one pair of matching surrounding quotes. */
std::string unquote(const std::string &s);

/* Double-quoted Python literal; escapes `\`, `"`, newline and CR. */
std::string python_string(const std::string &s);

/* Each name through posix_path, expand_home, python_string; space-joined. */
std::string drop_text(const std::string &home, const std::vector<std::string> &names);

} // namespace pyc
