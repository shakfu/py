// SPDX-License-Identifier: MIT
/* test.h -- minimal test harness: TEST(name), CHECK, CHECK_EQ, TEST_MAIN().
 * A failing CHECK reports file:line and the test keeps running; the exit code
 * is the number of failures (ctest treats non-zero as failure). */

#pragma once

#include <cstdio>
#include <sstream>
#include <string>
#include <vector>

namespace test {

struct Case {
	const char *name;
	void (*fn)();
};

inline std::vector<Case> &cases() {
	static std::vector<Case> v;
	return v;
}

inline int &failures() {
	static int n = 0;
	return n;
}

struct Register {
	Register(const char *name, void (*fn)()) { cases().push_back(Case{name, fn}); }
};

template <class T> std::string show(const T &v) {
	std::ostringstream o;
	o << v;
	return o.str();
}
inline std::string show(const std::string &v) {
	std::string o = "\"";
	for (char c : v)
		o += c == '\n' ? std::string("\\n") : std::string(1, c);
	return o + "\"";
}

} // namespace test

#define TEST(name)                                                                                                     \
	static void test_##name();                                                                                         \
	static test::Register reg_##name(#name, test_##name);                                                              \
	static void test_##name()

#define CHECK(cond)                                                                                                    \
	do {                                                                                                               \
		if (!(cond)) {                                                                                                 \
			std::fprintf(stderr, "%s:%d: CHECK(%s) failed\n", __FILE__, __LINE__, #cond);                             \
			++test::failures();                                                                                        \
		}                                                                                                              \
	} while (0)

#define CHECK_EQ(a, b)                                                                                                 \
	do {                                                                                                               \
		const auto va_ = (a);                                                                                          \
		const auto vb_ = (b);                                                                                          \
		if (!(va_ == vb_)) {                                                                                           \
			std::fprintf(stderr, "%s:%d: CHECK_EQ(%s, %s): %s != %s\n", __FILE__, __LINE__, #a, #b,                     \
						 test::show(va_).c_str(), test::show(vb_).c_str());                                            \
			++test::failures();                                                                                        \
		}                                                                                                              \
	} while (0)

#define TEST_MAIN()                                                                                                    \
	int main() {                                                                                                       \
		for (const test::Case &c : test::cases()) {                                                                    \
			const int before = test::failures();                                                                       \
			c.fn();                                                                                                    \
			std::printf("%s %s\n", test::failures() == before ? "ok  " : "FAIL", c.name);                              \
		}                                                                                                              \
		return test::failures();                                                                                       \
	}
