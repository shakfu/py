// SPDX-License-Identifier: MIT
/* convert.h -- LSP JSON results -> py_code values, and file URIs.
 * `text` is the document text the server answered for. */

#pragma once

#include "core/completion_item.h"
#include "core/python_env.h"
#include "lsp/types.h"

#include <nlohmann/json_fwd.hpp>

#include <string>
#include <vector>

namespace pyc::lsp {

/* `CompletionItem[]` or `CompletionList`; at most `limit` items. */
std::vector<CompletionItem> completion_items(const nlohmann::json &result, const std::string &text, bool utf16,
											 size_t limit = 200);
/* `SignatureHelp` or null. */
Signature signature_help(const nlohmann::json &result, bool utf16);
/* The `diagnostics` array of `textDocument/publishDiagnostics`. */
std::vector<Diagnostic> diagnostics(const nlohmann::json &list, const std::string &text, bool utf16);

/* Server settings for `env`, keyed by section: `python` and `basedpyright` (pyright
 * family). Empty object when env is empty. */
nlohmann::json python_settings(const PythonEnv &env);
/* `initializationOptions` for `env`: `pylsp` (jedi plugin) and `workspace`
 * (jedi-language-server). Null when env is empty. */
nlohmann::json python_init_options(const PythonEnv &env);
/* Result of `workspace/configuration`: per item, its dotted `section` looked up in
 * `settings` (null when absent), or all of `settings` when no section is given. */
nlohmann::json configuration_reply(const nlohmann::json &settings, const nlohmann::json &params);

std::string path_to_uri(const std::string &path); /* file:// + percent-encoded path */
std::string uri_to_path(const std::string &uri);  /* "" if not a file URI */

} // namespace pyc::lsp
