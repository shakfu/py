// SPDX-License-Identifier: MIT
/* python_env.cpp -- see python_env.h. */

#include "core/python_env.h"

#include <algorithm>

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

namespace pyc {

namespace {

bool is_dir(const std::string &p) {
	struct stat s;
	return ::stat(p.c_str(), &s) == 0 && S_ISDIR(s.st_mode);
}

bool is_exe(const std::string &p) {
	struct stat s;
	return ::stat(p.c_str(), &s) == 0 && S_ISREG(s.st_mode) && ::access(p.c_str(), X_OK) == 0;
}

std::string parent(std::string p) {
	while (p.size() > 1 && p.back() == '/')
		p.pop_back();
	const size_t i = p.find_last_of('/');
	return i == std::string::npos || i == 0 ? "/" : p.substr(0, i);
}

/* <lib>/python3.*\/site-packages, sorted so the result is stable. */
std::vector<std::string> site_packages(const std::string &lib) {
	std::vector<std::string> out;
	DIR *d = ::opendir(lib.c_str());
	if (!d)
		return out;
	while (const dirent *e = ::readdir(d)) {
		const std::string name = e->d_name;
		const std::string sp = lib + "/" + name + "/site-packages";
		if (name.rfind("python3", 0) == 0 && is_dir(sp))
			out.push_back(sp);
	}
	::closedir(d);
	std::sort(out.begin(), out.end());
	return out;
}

} // namespace

PythonEnv bundled_python(const std::string &bundle) {
	PythonEnv env;
	if (bundle.empty() || !is_dir(bundle))
		return env;
	const std::string res = bundle + "/Contents/Resources";
	const std::string fw = "/Python.framework/Versions/Current/bin/python3";
	if (is_dir(res + "/stubs"))
		env.paths.push_back(res + "/stubs");
	if (is_exe(res + fw))
		env.executable = res + fw;
	else if (is_exe(parent(parent(bundle)) + "/support" + fw))
		env.executable = parent(parent(bundle)) + "/support" + fw;
	else
		for (auto &sp : site_packages(res + "/lib"))
			env.paths.push_back(sp);
	return env;
}

} // namespace pyc
