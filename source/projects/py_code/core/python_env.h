// SPDX-License-Identifier: MIT
/* python_env.h -- what a language server should analyze against, found from
 * the py external's bundle (py.mxo) for each build variant. */

#pragma once

#include <string>
#include <vector>

namespace pyc {

struct PythonEnv {
	std::string executable;         /* interpreter the server queries for sys.path; may be empty */
	std::vector<std::string> paths; /* extra import roots: api stubs, bundled site-packages */
	bool empty() const { return executable.empty() && paths.empty(); }
	bool operator==(const PythonEnv &o) const { return executable == o.executable && paths == o.paths; }
};

/* `bundle` is the absolute path of py.mxo; only existing paths are returned.
 *   stubs:         <bundle>/Contents/Resources/stubs
 *   framework-ext: <bundle>/Contents/Resources/Python.framework/Versions/Current/bin/python3
 *   framework-pkg: <package>/support/Python.framework/Versions/Current/bin/python3
 *   shared/static: <bundle>/Contents/Resources/lib/python3.*\/site-packages (no executable ships)
 * <package> is two levels above the bundle (<package>/externals/py.mxo). */
PythonEnv bundled_python(const std::string &bundle);

} // namespace pyc
