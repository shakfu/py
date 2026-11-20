// SPDX-License-Identifier: MIT
#include "lsp/framing.h"

#include "test.h"

using pyc::lsp::encode;
using pyc::lsp::Framer;

namespace {
std::vector<std::string> drain(Framer &f) {
	std::vector<std::string> out;
	std::string s;
	while (f.next(&s))
		out.push_back(s);
	return out;
}
} // namespace

TEST(framing_encode_round_trip) {
	const std::string body = "{\"jsonrpc\":\"2.0\",\"id\":1,\"result\":\"\xc3\xa4\"}";
	const std::string framed = encode(body);
	CHECK_EQ(framed, "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body);
	Framer f;
	f.feed(framed.data(), framed.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(1));
	if (v.size() == 1)
		CHECK_EQ(v[0], body);
	CHECK_EQ(f.buffered(), size_t(0));
}

TEST(framing_split_chunks) {
	const std::string framed = encode("{\"a\":1}") + encode("{\"b\":22}");
	for (size_t step = 1; step <= 7; ++step) {
		Framer f;
		std::vector<std::string> got;
		for (size_t i = 0; i < framed.size(); i += step) {
			f.feed(framed.data() + i, std::min(step, framed.size() - i));
			for (auto &s : drain(f))
				got.push_back(s);
		}
		CHECK_EQ(got.size(), size_t(2));
		if (got.size() == 2) {
			CHECK_EQ(got[0], std::string("{\"a\":1}"));
			CHECK_EQ(got[1], std::string("{\"b\":22}"));
		}
	}
}

TEST(framing_incomplete_body_waits) {
	Framer f;
	const std::string framed = encode("{\"abc\":1}");
	f.feed(framed.data(), framed.size() - 1);
	std::string s;
	CHECK(!f.next(&s));
	f.feed(framed.data() + framed.size() - 1, 1);
	CHECK(f.next(&s));
	CHECK_EQ(s, std::string("{\"abc\":1}"));
}

TEST(framing_garbage_preamble) {
	Framer f;
	const std::string in = "Last login: today\nwelcome\n\nmore noise" + encode("{}") + encode("[1]");
	f.feed(in.data(), in.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(2));
	if (v.size() == 2) {
		CHECK_EQ(v[0], std::string("{}"));
		CHECK_EQ(v[1], std::string("[1]"));
	}
}

TEST(framing_two_messages_one_chunk) {
	Framer f;
	const std::string in = encode("{\"x\":1}") + encode("{\"y\":2}");
	f.feed(in.data(), in.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(2));
}

TEST(framing_lf_separators_and_headers) {
	Framer f;
	const std::string in = "content-length: 2\nContent-Type: application/vscode-jsonrpc\n\n{}"
						   "Content-Type: x\r\nCONTENT-LENGTH:3\r\n\r\n[1]";
	f.feed(in.data(), in.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(2));
	if (v.size() == 2) {
		CHECK_EQ(v[0], std::string("{}"));
		CHECK_EQ(v[1], std::string("[1]"));
	}
}

TEST(framing_body_may_contain_blank_lines) {
	Framer f;
	const std::string body = "{\"s\":\"a\n\nb\r\n\r\nc\"}";
	const std::string in = encode(body) + encode("{}");
	f.feed(in.data(), in.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(2));
	if (v.size() == 2)
		CHECK_EQ(v[0], body);
}

TEST(framing_bad_header_skipped) {
	Framer f;
	const std::string in = "Content-Length: abc\r\n\r\n" + encode("{}");
	f.feed(in.data(), in.size());
	auto v = drain(f);
	CHECK_EQ(v.size(), size_t(1));
	if (v.size() == 1)
		CHECK_EQ(v[0], std::string("{}"));
}
