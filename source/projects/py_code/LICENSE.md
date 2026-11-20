# py_code licensing

All `py_code` sources are MIT-licensed, like the rest of this repository.
Each file states its license in an SPDX header.

| Files | License | Origin |
|-|-|-|
| `py_code*.cpp`, `py_syntax.*`, `core/`, `lsp/`, `platform/`, `tests/` | MIT | this repository |
| `thirdparty/tree-sitter/` | MIT | [tree-sitter](https://github.com/tree-sitter/tree-sitter) |
| `thirdparty/tree-sitter-python/` | MIT | [tree-sitter-python](https://github.com/tree-sitter/tree-sitter-python) 0.25.0 |
| `thirdparty/nlohmann/` | MIT | [nlohmann/json](https://github.com/nlohmann/json) 3.12.0 |

An earlier version (commits `bfa5048` and `d96cb1a`) was derived from `sc_code` in
[gus74vv/sc3-tilde](https://github.com/gus74vv/sc3-tilde) (GPL-3.0-only).
The current sources were reimplemented from a behavioural spec,
`docs/dev/py_code-spec.md`, by authors who did not read the `sc_code` sources.
