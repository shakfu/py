// SPDX-License-Identifier: MIT
#include "lsp/convert.h"

#include "test.h"

#include <nlohmann/json.hpp>

using namespace pyc::lsp;
using json = nlohmann::json;

TEST(convert_completion_array) {
	/* pylsp shape: insertText, no textEdit. */
	const json r = json::parse(R"J([
		{"label": "pathconf(path, name)", "kind": 3, "insertText": "pathconf", "detail": "os",
		 "sortText": "apathconf", "filterText": "pathconf"},
		{"label": "path", "kind": 9},
		"junk"
	])J");
	const auto v = completion_items(r, "os.pa", true);
	CHECK_EQ(v.size(), size_t(2));
	if (v.size() != 2)
		return;
	CHECK_EQ(v[0].label, std::string("pathconf(path, name)"));
	CHECK_EQ(v[0].insert, std::string("pathconf"));
	CHECK_EQ(v[0].detail, std::string("os"));
	CHECK_EQ(v[0].sort, std::string("apathconf"));
	CHECK_EQ(v[0].filter, std::string("pathconf"));
	CHECK_EQ(v[0].kind, 3);
	CHECK_EQ(v[0].replace_start, -1);
	CHECK_EQ(v[0].replace_end, -1);
	CHECK_EQ(v[1].insert, std::string("path"));
	CHECK_EQ(v[1].kind, 9);
}

TEST(convert_completion_list_text_edit) {
	/* Line 1 is "\xc3\xa4\xf0\x9f\x98\x80.ab": the edit covers "ab" at UTF-16 4..6. */
	const std::string text = "x\n\xc3\xa4\xf0\x9f\x98\x80.ab";
	const json r = json::parse(R"J({"isIncomplete": false, "items": [
		{"label": "abc", "textEdit": {"range": {"start": {"line": 1, "character": 4},
		                                        "end": {"line": 1, "character": 6}}, "newText": "abc()"},
		 "insertText": "ignored"},
		{"label": "abd", "textEdit": {"insert": {"start": {"line": 1, "character": 4},
		                                         "end": {"line": 1, "character": 5}},
		                              "replace": {"start": {"line": 1, "character": 4},
		                                          "end": {"line": 1, "character": 6}}, "newText": "abd"}}
	]})J");
	const auto v = completion_items(r, text, true);
	CHECK_EQ(v.size(), size_t(2));
	if (v.size() != 2)
		return;
	CHECK_EQ(v[0].insert, std::string("abc()"));
	CHECK_EQ(v[0].replace_start, 9);
	CHECK_EQ(v[0].replace_end, 11);
	CHECK_EQ(v[1].insert, std::string("abd"));
	CHECK_EQ(v[1].replace_start, 9);
	CHECK_EQ(v[1].replace_end, 10);
	/* The same range in UTF-8 units. */
	const json r8 = json::parse(R"J([{"label": "abc", "textEdit": {"range": {"start": {"line": 1, "character": 7},
		"end": {"line": 1, "character": 9}}, "newText": "abc"}}])J");
	const auto v8 = completion_items(r8, text, false);
	CHECK_EQ(v8.size(), size_t(1));
	if (!v8.empty()) {
		CHECK_EQ(v8[0].replace_start, 9);
		CHECK_EQ(v8[0].replace_end, 11);
	}
}

TEST(convert_completion_limit_and_null) {
	json arr = json::array();
	for (int i = 0; i < 300; ++i) {
		char sort[8];
		std::snprintf(sort, sizeof sort, "%03d", 299 - i);
		arr.push_back({{"label", "n" + std::to_string(i)}, {"sortText", sort}});
	}
	const auto v = completion_items(arr, "", true);
	CHECK_EQ(v.size(), size_t(200));
	if (!v.empty()) /* the best sortText survives the cut */
		CHECK_EQ(v[0].label, std::string("n299"));
	CHECK_EQ(completion_items(arr, "", true, 5).size(), size_t(5));
	CHECK(completion_items(json(), "", true).empty());
	CHECK(completion_items(json::object(), "", true).empty());
	/* Under the limit, server order is kept. */
	const auto small = completion_items(json::parse(R"J([{"label":"b","sortText":"2"},{"label":"a","sortText":"1"}])J"),
										"", true);
	CHECK_EQ(small.size(), size_t(2));
	if (small.size() == 2)
		CHECK_EQ(small[0].label, std::string("b"));
}

TEST(convert_signature_offsets) {
	/* "f(\xc3\xa4: int, b)": UTF-16 [2,8] and [10,11] are bytes [2,9] and [11,12]. */
	const json r = {{"signatures",
					 {{{"label", "f(\xc3\xa4: int, b)"},
					   {"parameters", {{{"label", {2, 8}}}, {{"label", {10, 11}}}}},
					   {"activeParameter", 1}}}},
					{"activeSignature", 0},
					{"activeParameter", 0}};
	const Signature s = signature_help(r, true);
	CHECK_EQ(s.label, std::string("f(\xc3\xa4: int, b)"));
	CHECK_EQ(s.params.size(), size_t(2));
	if (s.params.size() == 2) {
		CHECK_EQ(s.params[0].first, 2);
		CHECK_EQ(s.params[0].second, 9);
		CHECK_EQ(s.params[1].first, 11);
		CHECK_EQ(s.params[1].second, 12);
	}
	CHECK_EQ(s.active, 1); /* the signature's activeParameter wins */
	const json r8 = {{"signatures",
					  {{{"label", "f(\xc3\xa4: int, b)"}, {"parameters", {{{"label", {2, 9}}}, {{"label", {11, 12}}}}}}}}};
	const Signature s8 = signature_help(r8, false);
	if (s8.params.size() == 2) {
		CHECK_EQ(s8.params[0].second, 9);
		CHECK_EQ(s8.params[1].first, 11);
	}
	CHECK_EQ(s8.active, 0); /* omitted everywhere: 0 */
}

TEST(convert_signature_string_labels) {
	/* pylsp shape. */
	const json r = json::parse(R"J({"signatures": [{"label": "len(obj: Sized, /) -> int",
		"parameters": [{"label": "obj"}]}], "activeSignature": 0, "activeParameter": 0})J");
	const Signature s = signature_help(r, true);
	CHECK_EQ(s.label, std::string("len(obj: Sized, /) -> int"));
	CHECK_EQ(s.params.size(), size_t(1));
	if (s.params.size() == 1) {
		CHECK_EQ(s.params[0].first, 4);
		CHECK_EQ(s.params[0].second, 7);
	}
	CHECK_EQ(s.active, 0);
	/* Sequential search, word boundaries, and a name equal to the function's. */
	const json r2 = json::parse(R"J({"signatures": [{"label": "a(aa, a, b=a)",
		"parameters": [{"label": "aa"}, {"label": "a"}, {"label": "b=a"}, {"label": "zz"}]}],
		"activeParameter": 2})J");
	const Signature s2 = signature_help(r2, true);
	CHECK_EQ(s2.params.size(), size_t(4));
	if (s2.params.size() == 4) {
		CHECK_EQ(s2.params[0].first, 2);
		CHECK_EQ(s2.params[1].first, 6);
		CHECK_EQ(s2.params[1].second, 7);
		CHECK_EQ(s2.params[2].first, 9);
		CHECK_EQ(s2.params[2].second, 12);
		CHECK_EQ(s2.params[3].first, -1);
	}
	CHECK_EQ(s2.active, 2);
}

TEST(convert_signature_active_and_empty) {
	const json base = json::parse(R"J({"signatures": [{"label": "one()"},
		{"label": "two(x, y)", "parameters": [{"label": "x"}, {"label": "y"}]}], "activeSignature": 1})J");
	json r = base;
	r["activeParameter"] = 1;
	CHECK_EQ(signature_help(r, true).active, 1);
	CHECK_EQ(signature_help(r, true).label, std::string("two(x, y)"));
	r["activeParameter"] = 5;
	CHECK_EQ(signature_help(r, true).active, -1); /* past the end: no highlight */
	r["activeParameter"] = nullptr;
	CHECK_EQ(signature_help(r, true).active, -1);
	r["signatures"][1]["activeParameter"] = 0; /* overrides the top-level null */
	CHECK_EQ(signature_help(r, true).active, 0);
	r = base;
	r["activeSignature"] = 7; /* out of range: first signature */
	const Signature first = signature_help(r, true);
	CHECK_EQ(first.label, std::string("one()"));
	CHECK_EQ(first.active, -1); /* no parameters */
	CHECK(signature_help(json(), true).label.empty());
	CHECK(signature_help(json::parse(R"J({"signatures": []})J"), true).label.empty());
}

TEST(convert_diagnostics) {
	const std::string text = "import os\n\xf0\x9f\x98\x80 = x\n";
	const json list = json::parse(R"J([
		{"range": {"start": {"line": 0, "character": 0}, "end": {"line": 0, "character": 9}},
		 "severity": 2, "message": "'os' imported but unused", "source": "pyflakes"},
		{"range": {"start": {"line": 1, "character": 5}, "end": {"line": 1, "character": 6}},
		 "message": "undefined name 'x'"},
		{"range": {"start": {"line": 1, "character": 2}, "end": {"line": 1, "character": 2}},
		 "severity": 9, "message": "empty"},
		{"message": "no range"}
	])J");
	const auto d = diagnostics(list, text, true);
	CHECK_EQ(d.size(), size_t(3));
	if (d.size() != 3)
		return;
	CHECK_EQ(d[0].start, 0);
	CHECK_EQ(d[0].end, 9);
	CHECK_EQ(d[0].severity, 2);
	CHECK_EQ(d[0].message, std::string("'os' imported but unused"));
	CHECK_EQ(d[1].start, 17);
	CHECK_EQ(d[1].end, 18);
	CHECK_EQ(d[1].severity, 1); /* omitted: error */
	CHECK_EQ(d[2].start, 14);
	CHECK_EQ(d[2].end, 14);
	CHECK_EQ(d[2].severity, 4); /* clamped */
	CHECK(diagnostics(json(), text, true).empty());
}

TEST(convert_uris) {
	CHECK_EQ(path_to_uri("/Users/a b/x_1-2.~py"), std::string("file:///Users/a%20b/x_1-2.~py"));
	CHECK_EQ(path_to_uri("/t/\xc3\xa4:#?%.py"), std::string("file:///t/%C3%A4%3A%23%3F%25.py"));
	CHECK_EQ(uri_to_path("file:///t/%C3%A4%3a%23%3F%25.py"), std::string("/t/\xc3\xa4:#?%.py"));
	CHECK_EQ(uri_to_path("file://localhost/a%20b"), std::string("/a b"));
	CHECK_EQ(uri_to_path("untitled:x"), std::string(""));
	const std::string p = "/tmp/py code \xc3\xa9/a.py";
	CHECK_EQ(uri_to_path(path_to_uri(p)), p);
}

TEST(convert_python_settings) {
	pyc::PythonEnv env;
	CHECK_EQ(python_settings(env).dump(), std::string("{}"));
	CHECK(python_init_options(env).is_null());
	env.paths = {"/s", "/t"};
	CHECK_EQ(python_settings(env).dump(),
			 std::string(R"({"basedpyright":{"analysis":{"extraPaths":["/s","/t"]}},)"
						 R"("python":{"analysis":{"extraPaths":["/s","/t"]}}})"));
	CHECK_EQ(python_init_options(env).dump(),
			 std::string(R"({"pylsp":{"plugins":{"jedi":{"extra_paths":["/s","/t"]}}},)"
						 R"("workspace":{"extraPaths":["/s","/t"]}})"));
}

TEST(convert_configuration_reply) {
	const json settings = json::parse(R"({"python": {"analysis": {"extraPaths": ["/s"]}}, "n": 1})");
	const json params = json::parse(R"({"items": [
		{"section": "python.analysis"}, {"section": "python.analysis.extraPaths"},
		{"section": "n.x"}, {"section": "python."}, {"section": "zzz"}, {}, "junk"]})");
	CHECK_EQ(configuration_reply(settings, params).dump(),
			 std::string(R"([{"extraPaths":["/s"]},["/s"],null,null,null,)") + settings.dump() + ",null]");
	CHECK_EQ(configuration_reply(json::object(), json::parse(R"({"items": [{}]})")).dump(), std::string("[null]"));
	CHECK_EQ(configuration_reply(settings, json::object()).dump(), std::string("[]"));
}
