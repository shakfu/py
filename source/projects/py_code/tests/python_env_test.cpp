// SPDX-License-Identifier: MIT
/* python_env_test.cpp -- bundled_python against fake bundle layouts. */

#include "test.h"

#include "core/python_env.h"

#include <cstdlib>
#include <fstream>
#include <string>

#include <sys/stat.h>
#include <unistd.h>

using namespace pyc;
using std::string;

namespace {

/* A fresh <tmp>/pkg directory; the package root of the fake layout. */
string fresh_package() {
	char tmpl[] = "/tmp/py_code_env_XXXXXX";
	const char *dir = ::mkdtemp(tmpl);
	CHECK(dir != nullptr);
	return string(dir ? dir : "/tmp") + "/pkg";
}

void mkdirs(const string &p) {
	for (size_t i = 1; i <= p.size(); ++i)
		if (i == p.size() || p[i] == '/')
			::mkdir(p.substr(0, i).c_str(), 0755);
}

void touch(const string &p, bool exe) {
	mkdirs(p.substr(0, p.find_last_of('/')));
	std::ofstream(p) << "#!/bin/sh\n";
	::chmod(p.c_str(), exe ? 0755 : 0644);
}

const char *kFw = "/Python.framework/Versions/Current/bin/python3";

} // namespace

TEST(python_env_missing_bundle) {
	CHECK(bundled_python("").empty());
	CHECK(bundled_python("/nonexistent/py.mxo").empty());
}

TEST(python_env_local_build) {
	const string b = fresh_package() + "/externals/py.mxo";
	mkdirs(b + "/Contents/MacOS");
	CHECK(bundled_python(b).empty());
}

TEST(python_env_stubs) {
	const string b = fresh_package() + "/externals/py.mxo";
	mkdirs(b + "/Contents/Resources/stubs");
	const PythonEnv e = bundled_python(b);
	CHECK_EQ(e.executable, string());
	CHECK_EQ(e.paths.size(), size_t(1));
	if (e.paths.size() == 1)
		CHECK_EQ(e.paths[0], b + "/Contents/Resources/stubs");
}

TEST(python_env_framework_ext) {
	const string b = fresh_package() + "/externals/py.mxo";
	const string res = b + "/Contents/Resources";
	mkdirs(res + "/stubs");
	touch(res + kFw, true);
	mkdirs(res + "/lib/python3.13/site-packages"); /* ignored: the interpreter knows its own */
	const PythonEnv e = bundled_python(b);
	CHECK_EQ(e.executable, res + kFw);
	CHECK_EQ(e.paths.size(), size_t(1));
}

TEST(python_env_framework_ext_not_executable) {
	const string b = fresh_package() + "/externals/py.mxo";
	touch(b + "/Contents/Resources" + kFw, false);
	CHECK_EQ(bundled_python(b).executable, string());
}

TEST(python_env_framework_pkg) {
	const string pkg = fresh_package();
	const string b = pkg + "/externals/py.mxo";
	mkdirs(b + "/Contents/MacOS");
	touch(pkg + "/support" + kFw, true);
	CHECK_EQ(bundled_python(b).executable, pkg + "/support" + kFw);
	CHECK_EQ(bundled_python(b + "/").executable, pkg + "/support" + kFw);
}

TEST(python_env_shared_or_static_ext) {
	const string b = fresh_package() + "/externals/py.mxo";
	const string lib = b + "/Contents/Resources/lib";
	mkdirs(lib + "/python3.14/site-packages");
	mkdirs(lib + "/python3.13/site-packages");
	mkdirs(lib + "/python3.12"); /* no site-packages */
	mkdirs(lib + "/other/site-packages");
	const PythonEnv e = bundled_python(b);
	CHECK_EQ(e.executable, string());
	CHECK_EQ(e.paths.size(), size_t(2));
	if (e.paths.size() == 2) {
		CHECK_EQ(e.paths[0], lib + "/python3.13/site-packages");
		CHECK_EQ(e.paths[1], lib + "/python3.14/site-packages");
	}
}
