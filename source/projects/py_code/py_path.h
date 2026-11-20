// SPDX-License-Identifier: MIT
/* py_path.h -- path handling for Finder drops and `@snippets`.
 *
 *   - `path_to_posix` drops Max's volume prefix (`Macintosh HD:/Users/...`).
 *   - `path_to_absolute` also expands `~`.
 *   - `quote_py_string` makes a double-quoted Python string literal.
 *   - `unquote_path` drops one pair of surrounding quotes from typed input. */

#pragma once

#include <string>
#include <vector>

namespace py_code {

/* Absolute POSIX form of a Max pathname: the volume prefix (`Macintosh HD:`) is
 * dropped, so `Macintosh HD:/Users/g/x.wav` -> `/Users/g/x.wav`. A name relative to a
 * Max search path stays as is (there is nothing to join it to here). Used to check that
 * a dropped item really is a file (an object drag has no such path). */
std::string path_to_posix(const std::string &in);

/* The same, plus `~` -> `home`, which is what the editor inserts:
 *   "Macintosh HD:/Users/g/x.wav"  + home "/Users/g" -> "/Users/g/x.wav"
 *   "~/Documents/x.wav"                              -> "/Users/g/Documents/x.wav"
 *   "~"                                              -> "/Users/g"
 *   "x.wav" (relative to a search path)              -> "x.wav"   (left untouched)
 *   "~other/x.wav"                                   -> "~other/x.wav"
 * A trailing `/` in `home` is ignored; an empty `home` disables the expansion; an empty
 * input stays empty. */
std::string path_to_absolute(const std::string &home, const std::string &in);

/* The path without one pair of surrounding quotes: `'...'` or `"..."`, with the whitespace
 * around it trimmed. Only a FULL wrap counts, so `a"b.wav` and `'abc` stay as they are
 * (the open error then shows the real text), and `''` becomes empty (= no path, which
 * is how `@snippets ""` unloads the file). The inside is verbatim: no escapes. */
std::string unquote_path(const std::string &in);

/* The path as a Python string literal: `"..."` with `\` and `"` escaped. */
std::string quote_py_string(const std::string &path);

/* The text inserted by a drop: every path made absolute (see `path_to_absolute`),
 * quoted, separated by a space (dropping two files in a row gives
 * `"/Users/g/a.wav" "/Users/g/b.wav"`). */
std::string dropped_text(const std::string &home, const std::vector<std::string> &names);

} // namespace py_code
